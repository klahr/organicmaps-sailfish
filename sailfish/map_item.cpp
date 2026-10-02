#include "sailfish/map_item.hpp"

#include "sailfish/framework_access.hpp"
#include "sailfish/place_page.hpp"

#include "map/framework.hpp"

#include "drape_frontend/user_event_stream.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/map_style.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <QGuiApplication>
#include <QOpenGLContext>
#include <QQuickWindow>
#include <QSGSimpleTextureNode>
#include <QScreen>
#include <QTouchEvent>

namespace sailfish
{
namespace
{
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
{
  setFlag(ItemHasContents);
  setAcceptedMouseButtons(Qt::LeftButton);

  connect(this, &QQuickItem::windowChanged, this, &MapItem::OnWindowChanged);
  connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, &MapItem::OnApplicationStateChanged);

  // Drape renders on its own threads, so poll for new frames like the desktop map widget.
  m_updateTimer.setInterval(1000 / 60);
  connect(&m_updateTimer, &QTimer::timeout, this, &QQuickItem::update);
}

MapItem::~MapItem()
{
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
  return mask;
}

bool MapItem::darkStyle() const
{
  return MapStyleIsDark(m_framework.GetMapStyle());
}

void MapItem::setLayerEnabled(int layer, bool enabled)
{
  switch (layer)
  {
  case Outdoors:
  {
    Framework::SaveOutdoorsEnabled(enabled);
    bool const dark = darkStyle();
    if (enabled)
      m_framework.SetMapStyle(dark ? MapStyleOutdoorsDark : MapStyleOutdoorsLight);
    else
      m_framework.SetMapStyle(dark ? MapStyleDefaultDark : MapStyleDefaultLight);
    break;
  }
  case Isolines:
    m_framework.GetIsolinesManager().SetEnabled(enabled);
    Framework::SaveIsolinesEnabled(enabled);
    break;
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
    m_locationService->Stop();
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
  m_framework.CreateDrapeEngine(make_ref(m_contextFactory), std::move(p));
  m_framework.EnterForeground();
  m_contextFactory->WaitForInitialization(nullptr);

  m_updateTimer.start();
}

void MapItem::Resize(int width, int height)
{
  m_framework.OnSize(width, height);
  if (!m_skin)
    return;

  m_skin->Resize(width, height);
  gui::TWidgetsLayoutInfo layout;
  m_skin->ForEach([&layout](gui::EWidget w, gui::Position const & pos) { layout[w] = pos.m_pixelPivot; });
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
