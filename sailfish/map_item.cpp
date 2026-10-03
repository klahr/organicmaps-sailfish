#include "sailfish/map_item.hpp"

#include "sailfish/app_info.hpp"
#include "sailfish/app_settings.hpp"
#include "sailfish/framework_access.hpp"
#include "sailfish/place_page.hpp"
#include "sailfish/routing.hpp"

#include "map/framework.hpp"

#include "drape_frontend/user_event_stream.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/map_style.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/storage.hpp"
#include "storage/storage_helpers.hpp"

#include "geometry/angles.hpp"
#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/math.hpp"

#include <QCompass>
#include <QGuiApplication>
#include <QNetworkConfigurationManager>
#include <QOpenGLContext>
#include <QQuickWindow>
#include <QSGSimpleTextureNode>
#include <QScreen>
#include <QTouchEvent>

#include <optional>

namespace sailfish
{
namespace
{
static_assert(MapItem::PendingPosition == static_cast<int>(location::PendingPosition));
static_assert(MapItem::NotFollowNoPosition == static_cast<int>(location::NotFollowNoPosition));
static_assert(MapItem::NotFollow == static_cast<int>(location::NotFollow));
static_assert(MapItem::Follow == static_cast<int>(location::Follow));
static_assert(MapItem::FollowAndRotate == static_cast<int>(location::FollowAndRotate));

// Drape presents into a power-of-two framebuffer and reports the used part as a normalized
// rect, so the texture wrapper is recreated whenever the framebuffer or its size changes.
class MapTextureNode : public QSGSimpleTextureNode
{
public:
  MapTextureNode() { setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically); }
  ~MapTextureNode() override { delete m_texture; }

  void Update(QQuickWindow * window, GLuint id, QSize const & textureSize, QSize const & usedSize)
  {
    if (!m_texture || id != m_id || textureSize != m_textureSize)
    {
      QSGTexture * texture = window->createTextureFromId(id, textureSize);
      setTexture(texture);
      delete m_texture;
      m_texture = texture;
      m_id = id;
      m_textureSize = textureSize;
    }
    setSourceRect(0, 0, usedSize.width(), usedSize.height());
  }

private:
  QSGTexture * m_texture = nullptr;
  GLuint m_id = 0;
  QSize m_textureSize;
};
}  // namespace

MapItem::MapItem(QQuickItem * parent)
  : QQuickItem(parent)
  , m_framework(GetFramework())
  , m_locationService(CreateDesktopLocationService(*this))
  , m_placePage(std::make_unique<PlacePage>(m_framework))
  , m_routing(std::make_unique<Routing>(m_framework))
  , m_compass(new QCompass(this))
{
  m_compass->setSkipDuplicates(true);
  connect(m_compass, &QCompass::readingChanged, this, &MapItem::OnCompassReading);

  setFlag(ItemHasContents);
  setAcceptedMouseButtons(Qt::LeftButton);

  connect(this, &QQuickItem::windowChanged, this, &MapItem::OnWindowChanged);
  connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, &MapItem::OnApplicationStateChanged);

  m_storageSlot = m_framework.GetStorage().Subscribe([this](storage::CountryId const &) {
    UpdateCurrentCountry();
  }, [this](storage::CountryId const &, downloader::Progress const &) { UpdateCurrentCountry(); });

  // Drape renders on its own threads, so poll for new frames like the desktop map widget.
  m_updateTimer.setInterval(1000 / 60);
  connect(&m_updateTimer, &QTimer::timeout, this, &QQuickItem::update);
}

MapItem::~MapItem()
{
  m_framework.GetStorage().Unsubscribe(m_storageSlot);
  m_locationService->Stop();
  if (!m_contextFactory)
    return;

  m_framework.SetMyPositionModeListener(nullptr);
  m_framework.EnterBackground();
  m_framework.SetRenderingDisabled(true);
  m_contextFactory->PrepareToShutdown();
  m_framework.DestroyDrapeEngine();
  m_contextFactory.reset();
}

void MapItem::zoomIn()
{
  m_framework.Scale(Framework::SCALE_MAG, true);
}

void MapItem::zoomOut()
{
  m_framework.Scale(Framework::SCALE_MIN, true);
}

void MapItem::switchMyPositionMode()
{
  if (m_contextFactory)
    m_framework.SwitchMyPositionNextMode();
}

int MapItem::enabledLayers() const
{
  int mask = 0;
  auto const set = [&mask](Layer layer, bool enabled)
  {
    if (enabled)
      mask |= 1 << layer;
  };
  set(Outdoors, Framework::LoadOutdoorsEnabled());
  set(Isolines, Framework::LoadIsolinesEnabled());
  set(Hiking, Framework::IsHikingEnabled());
  set(Cycling, Framework::IsCyclingEnabled());
  set(Subway, Framework::LoadTransitSchemeEnabled());
  set(Satellite, Framework::IsBackgroundTilesEnabled());
  return mask;
}

void MapItem::setLayerEnabled(int layer, bool enabled)
{
  switch (layer)
  {
  case Outdoors:
  {
    Framework::SaveOutdoorsEnabled(enabled);
    m_framework.SetMapStyle(BaseMapStyle(MapStyleIsDark(m_framework.GetMapStyle()), enabled));
    break;
  }
  case Isolines:
    m_framework.GetIsolinesManager().SetEnabled(enabled);
    Framework::SaveIsolinesEnabled(enabled);
    break;
  case Satellite: m_framework.SetBackgroundTilesEnabled(enabled); break;
  case Hiking: m_framework.SetHikingEnabled(enabled); break;
  case Cycling: m_framework.SetCyclingEnabled(enabled); break;
  case Subway:
    m_framework.GetTransitManager().EnableTransitSchemeMode(enabled);
    Framework::SaveTransitSchemeEnabled(enabled);
    break;
  default: ASSERT(false, ("Unknown layer", layer)); return;
  }
  emit layersChanged();
}

void MapItem::OnMyPositionModeChanged(location::EMyPositionMode mode)
{
  // Drape asks for a position by entering PendingPosition, either on start or from the button.
  if (mode == location::PendingPosition && !m_inBackground)
    m_locationService->Start();

  if (mode != m_myPositionMode)
  {
    m_myPositionMode = mode;
    emit myPositionModeChanged();
  }
}

void MapItem::OnLocationError(location::TLocationError errorCode)
{
  LOG(LWARNING, ("Location error:", errorCode));
  m_framework.OnLocationError(errorCode);
}

void MapItem::OnLocationUpdated(location::GpsInfo const & info)
{
  m_framework.OnLocationUpdate(info);
  m_placePage->UpdateDistance();
  m_routing->UpdateNavigation(info.m_speed);
}

bool MapItem::trackRecording() const
{
  return m_framework.IsTrackRecordingEnabled();
}

void MapItem::startTrackRecording()
{
  m_framework.StartTrackRecording();
  // Recording needs fixes even when the my position button is off.
  m_locationService->Start();
  emit trackRecordingChanged();
}

bool MapItem::isTrackRecordingEmpty() const
{
  return m_framework.IsTrackRecordingEmpty();
}

void MapItem::stopTrackRecording(QString const & saveAsName)
{
  // The same order as the Android track recording place page: save first, then stop.
  if (!saveAsName.isEmpty() && !m_framework.IsTrackRecordingEmpty())
    m_framework.SaveTrackRecordingWithName(saveAsName.toStdString());
  m_framework.StopTrackRecording();
  if (m_inBackground && !m_routing->navigating())
    m_locationService->Stop();
  emit trackRecordingChanged();
}

QString MapItem::myPositionShareText() const
{
  auto const position = m_framework.GetCurrentPosition();
  if (!position)
    return {};
  return QString::fromStdString(m_framework.GetShareDataForMyPosition(mercator::ToLatLon(*position)).m_text);
}

void MapItem::startChoosingPosition(bool business)
{
  // Like Android, the cross starts at the selected place and the map zooms in to it.
  std::optional<m2::PointD> position;
  if (m_framework.HasPlacePageInfo())
    position = m_framework.GetCurrentPlacePageInfo().GetMercator();
  m_placePage->close();
  m_framework.BlockTapEvents(true);
  m_framework.EnableChoosePositionMode(true, business /* enableBounds */, position ? &*position : nullptr,
                                       true /* shouldChangeViewport */);
  m_choosingPosition = true;
  emit choosingPositionChanged();
}

void MapItem::stopChoosingPosition()
{
  if (!m_choosingPosition)
    return;
  m_framework.EnableChoosePositionMode(false, false /* enableBounds */, nullptr, false /* shouldChangeViewport */);
  m_framework.BlockTapEvents(false);
  m_choosingPosition = false;
  emit choosingPositionChanged();
}

QVariantList MapItem::confirmChosenPosition()
{
  // Taken now: the viewport can still move while the category is picked.
  auto const center = m_framework.GetViewportCenter();
  if (!storage::IsPointCoveredByDownloadedMaps(center, m_framework.GetStorage(), m_framework.GetCountryInfoGetter()))
    return {};
  stopChoosingPosition();
  auto const latLon = mercator::ToLatLon(center);
  return {latLon.m_lat, latLon.m_lon};
}

void MapItem::OnCurrentCountryChanged(std::string const & countryId)
{
  m_currentCountryId = countryId;
  UpdateCurrentCountry();

  // The conditions of OnmapDownloader on Android: enabled, on Wi-Fi, in that region and with enough space.
  if (countryId.empty() || !AppSettings::IsAutoDownloadEnabled())
    return;
  auto & storage = m_framework.GetStorage();
  storage::NodeStatuses statuses;
  storage.GetNodeStatuses(countryId, statuses);
  if (statuses.m_status != storage::NodeStatus::NotDownloaded)
    return;
  auto const position = m_framework.GetCurrentPosition();
  if (!position || m_framework.GetCountryInfoGetter().GetRegionCountryId(*position) != countryId)
    return;
  if (QNetworkConfigurationManager().defaultConfiguration().bearerType() != QNetworkConfiguration::BearerWLAN)
    return;
  if (storage::IsEnoughSpaceForDownload(countryId, storage))
    storage.DownloadNode(countryId);
}

void MapItem::UpdateCurrentCountry()
{
  QVariantMap country;
  if (!m_currentCountryId.empty())
  {
    storage::NodeAttrs attrs;
    m_framework.GetStorage().GetNodeAttrs(m_currentCountryId, attrs);
    using storage::NodeStatus;
    switch (attrs.m_status)
    {
    case NodeStatus::NotDownloaded:
    case NodeStatus::Downloading:
    case NodeStatus::Applying:
    case NodeStatus::InQueue:
    case NodeStatus::Error:
    {
      auto const & progress = attrs.m_downloadingProgress;
      country["countryId"] = QString::fromStdString(m_currentCountryId);
      country["name"] = QString::fromStdString(attrs.m_nodeLocalName);
      country["size"] = FormatSize(static_cast<qint64>(attrs.m_mwmSize));
      country["status"] = static_cast<int>(attrs.m_status);
      country["progress"] =
          progress.m_bytesTotal > 0 ? static_cast<double>(progress.m_bytesDownloaded) / progress.m_bytesTotal : 0.0;
      break;
    }
    default: break;
    }
  }
  if (country != m_currentCountry)
  {
    m_currentCountry = country;
    emit currentCountryChanged();
  }
}

void MapItem::downloadCurrentCountry()
{
  if (m_currentCountryId.empty())
    return;
  auto & storage = m_framework.GetStorage();
  storage::NodeStatuses statuses;
  storage.GetNodeStatuses(m_currentCountryId, statuses);
  if (statuses.m_status == storage::NodeStatus::Error)
    storage.RetryDownloadNode(m_currentCountryId);
  else
    storage.DownloadNode(m_currentCountryId);
}

void MapItem::cancelCurrentCountry()
{
  if (!m_currentCountryId.empty())
    m_framework.GetStorage().CancelDownloadNode(m_currentCountryId);
}

void MapItem::OnCompassReading()
{
  auto const * reading = m_compass->reading();
  if (!m_contextFactory || !reading || !window() || !window()->screen())
    return;

  // The azimuth is measured at the top of the device; turn it with the UI like Android's
  // LocationUtils.correctCompassAngle(). Magnetic declination is not corrected there either.
  QScreen const * screen = window()->screen();
  int const rotation = screen->angleBetween(screen->nativeOrientation(), window()->contentOrientation());
  location::CompassInfo info;
  info.m_bearing = ang::AngleIn2PI(math::DegToRad(static_cast<double>(reading->azimuth() + rotation)));
  m_framework.OnCompassUpdate(info);
  m_placePage->SetNorth(info.m_bearing);
}

void MapItem::OnWindowChanged(QQuickWindow * window)
{
  if (window)
    CreateEngine();
}

void MapItem::OnApplicationStateChanged(Qt::ApplicationState state)
{
  // Sailfish keeps a minimized app running as its cover; stop drawing until it is active again.
  // Map downloads are handled by storage and keep going.
  bool const inBackground = state != Qt::ApplicationActive;
  if (!m_contextFactory || inBackground == m_inBackground)
    return;

  m_inBackground = inBackground;
  LOG(LINFO, (inBackground ? "Entering background" : "Entering foreground"));
  if (inBackground)
  {
    m_updateTimer.stop();
    // Track recording and navigation keep going in the background, like the Android foreground service.
    if (!m_framework.IsTrackRecordingEnabled() && !m_routing->navigating())
      m_locationService->Stop();
    m_compass->stop();
    m_framework.SetRenderingDisabled(false /* destroySurface */);
    m_framework.EnterBackground();
  }
  else
  {
    m_framework.EnterForeground();
    m_framework.SetRenderingEnabled();
    m_updateTimer.start();
    if (m_myPositionMode != location::NotFollowNoPosition)
      m_locationService->Start();
    m_compass->start();
  }
}

void MapItem::CreateEngine()
{
  if (m_contextFactory || !window() || width() <= 0 || height() <= 0)
    return;

  QOpenGLContext * shareContext = QOpenGLContext::globalShareContext();
  CHECK(shareContext, ("Qt::AA_ShareOpenGLContexts must be set before the application is created"));

  QScreen * screen = window()->screen();
  qreal const dpi = screen ? screen->physicalDotsPerInch() : 0;
  m_visualScale = dpi > 0 ? df::DPI2VS(dpi) : 2.0;
  LOG(LINFO, ("Screen DPI:", dpi, "visual scale:", m_visualScale));

  m_contextFactory = std::make_unique<qt::common::QtOGLContextFactory>(shareContext);

  Framework::DrapeCreationParams p;
  p.m_apiVersion = dp::ApiVersion::OpenGLES3;
  p.m_surfaceWidth = static_cast<int>(width());
  p.m_surfaceHeight = static_cast<int>(height());
  p.m_visualScale = static_cast<float>(m_visualScale);

  m_skin = std::make_unique<gui::Skin>(gui::ResolveGuiSkinFile("default"), m_visualScale);
  m_skin->Resize(p.m_surfaceWidth, p.m_surfaceHeight);
  m_skin->ForEach([&p](gui::EWidget widget, gui::Position const & pos) { p.m_widgetsInitInfo[widget] = pos; });

  m_framework.SetMyPositionModeListener([this](location::EMyPositionMode mode, bool /* routingActive */)
  { OnMyPositionModeChanged(mode); });
  m_framework.SetCurrentCountryChangedListener([this](storage::CountryId const & countryId)
  { OnCurrentCountryChanged(countryId); });
  m_framework.CreateDrapeEngine(make_ref(m_contextFactory), std::move(p));
  m_framework.EnterForeground();
  m_contextFactory->WaitForInitialization(nullptr);
  UpdateWidgetLayout();
  UpdateVisibleViewport();

  m_updateTimer.start();
  m_compass->start();
}

void MapItem::Resize(int width, int height)
{
  m_framework.OnSize(width, height);
  if (!m_skin)
    return;

  m_skin->Resize(width, height);
  UpdateWidgetLayout();
  UpdateVisibleViewport();
}

void MapItem::setViewportBottomInset(qreal inset)
{
  if (inset == m_viewportBottomInset)
    return;
  m_viewportBottomInset = inset;
  emit viewportBottomInsetChanged();
  UpdateVisibleViewport();
}

void MapItem::UpdateVisibleViewport()
{
  // Ignored by the framework until the drape engine exists.
  double const bottom = std::max(1.0, height() - m_viewportBottomInset);
  m_framework.SetVisibleViewport(m2::RectD(0, 0, width(), bottom));
}

void MapItem::setBottomWidgetsOffset(qreal offset)
{
  if (offset == m_bottomWidgetsOffset)
    return;
  m_bottomWidgetsOffset = offset;
  emit bottomWidgetsOffsetChanged();
  if (m_skin)
    UpdateWidgetLayout();
}

void MapItem::UpdateWidgetLayout()
{
  gui::TWidgetsLayoutInfo layout;
  m_skin->ForEach([&layout](gui::EWidget w, gui::Position const & pos) { layout[w] = pos.m_pixelPivot; });
  // Both are anchored to the bottom left corner. Like Map.updateRulerOffset() on Android, the ruler sits
  // a little higher than the attribution.
  double const offset = m_bottomWidgetsOffset;
  layout[gui::WIDGET_RULER].y -= offset + 8 * m_visualScale;
  layout[gui::WIDGET_COPYRIGHT].y -= offset;
  m_framework.SetWidgetLayout(std::move(layout));
}

void MapItem::geometryChanged(QRectF const & newGeometry, QRectF const & oldGeometry)
{
  QQuickItem::geometryChanged(newGeometry, oldGeometry);
  if (newGeometry.size() == oldGeometry.size())
    return;

  if (m_contextFactory)
    Resize(static_cast<int>(newGeometry.width()), static_cast<int>(newGeometry.height()));
  else
    CreateEngine();
}

QSGNode * MapItem::updatePaintNode(QSGNode * oldNode, UpdatePaintNodeData *)
{
  if (!m_contextFactory || !m_contextFactory->AcquireFrame())
    return oldNode;

  GLuint const textureId = m_contextFactory->GetTextureHandle();
  QRectF const & texRect = m_contextFactory->GetTexRect();
  if (textureId == 0 || texRect.width() <= 0 || texRect.height() <= 0)
    return oldNode;

  auto * node = static_cast<MapTextureNode *>(oldNode);
  if (!node)
    node = new MapTextureNode();

  QSize const usedSize(static_cast<int>(width()), static_cast<int>(height()));
  QSize const textureSize(qRound(usedSize.width() / texRect.width()), qRound(usedSize.height() / texRect.height()));
  node->Update(window(), textureId, textureSize, usedSize);
  node->setRect(boundingRect());
  return node;
}

void MapItem::touchEvent(QTouchEvent * event)
{
  if (!m_contextFactory)
    return;

  if (event->type() == QEvent::TouchCancel)
  {
    df::TouchEvent cancelEvent;
    cancelEvent.SetTouchType(df::TouchEvent::TOUCH_CANCEL);
    m_framework.TouchEvent(cancelEvent);
    return;
  }

  auto const & points = event->touchPoints();
  if (points.isEmpty())
    return;

  df::TouchEvent touchEvent;
  for (int i = 0; i < points.size() && i < 2; ++i)
  {
    df::Touch touch;
    touch.m_id = points[i].id();
    touch.m_location = m2::PointD(points[i].pos().x(), points[i].pos().y());
    if (i == 0)
      touchEvent.SetFirstTouch(touch);
    else
      touchEvent.SetSecondTouch(touch);
  }

  // Report which finger changed, matching the Android MotionEvent bridge.
  uint8_t maskedPointer = df::TouchEvent::INVALID_MASKED_POINTER;
  df::TouchEvent::ETouchType type = df::TouchEvent::TOUCH_MOVE;
  for (int i = 0; i < points.size() && i < 2; ++i)
  {
    if (points[i].state() == Qt::TouchPointPressed)
    {
      type = df::TouchEvent::TOUCH_DOWN;
      maskedPointer = static_cast<uint8_t>(i);
    }
    else if (points[i].state() == Qt::TouchPointReleased)
    {
      type = df::TouchEvent::TOUCH_UP;
      maskedPointer = static_cast<uint8_t>(i);
    }
  }
  touchEvent.SetTouchType(type);
  touchEvent.SetFirstMaskedPointer(maskedPointer);
  m_framework.TouchEvent(touchEvent);
  event->accept();
}

void MapItem::SendMouseTouch(QMouseEvent * event, int touchType)
{
  if (!m_contextFactory)
    return;

  df::Touch touch;
  touch.m_id = 0;
  touch.m_location = m2::PointD(event->localPos().x(), event->localPos().y());

  df::TouchEvent touchEvent;
  touchEvent.SetTouchType(static_cast<df::TouchEvent::ETouchType>(touchType));
  touchEvent.SetFirstTouch(touch);
  m_framework.TouchEvent(touchEvent);
  event->accept();
}

void MapItem::mousePressEvent(QMouseEvent * event)
{
  SendMouseTouch(event, df::TouchEvent::TOUCH_DOWN);
}

void MapItem::mouseMoveEvent(QMouseEvent * event)
{
  SendMouseTouch(event, df::TouchEvent::TOUCH_MOVE);
}

void MapItem::mouseReleaseEvent(QMouseEvent * event)
{
  SendMouseTouch(event, df::TouchEvent::TOUCH_UP);
}
}  // namespace sailfish
