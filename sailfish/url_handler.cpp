#include "sailfish/url_handler.hpp"

#include "sailfish/bookmarks_io.hpp"

#include "map/framework.hpp"
#include "map/mwm_url.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"

#include <QDBusConnection>
#include <QVariantMap>

namespace sailfish
{
namespace
{
QString const kService = QStringLiteral("app.organicmaps.organicmaps");
QString const kPath = QStringLiteral("/app/organicmaps");
// The zoom of searches and positions from links, like SEARCH_IN_VIEWPORT_ZOOM on Android.
int constexpr kLinkZoom = 16;
}  // namespace

UrlHandler::UrlHandler(Framework & framework, BookmarksIO & bookmarksIO, QObject * parent)
  : QObject(parent)
  , m_framework(framework)
  , m_bookmarksIO(bookmarksIO)
{
  m_retryTimer.setInterval(200);
  connect(&m_retryTimer, &QTimer::timeout, this, &UrlHandler::ProcessPending);
}

bool UrlHandler::RegisterOnDBus()
{
  auto bus = QDBusConnection::sessionBus();
  if (!bus.registerService(kService))
  {
    LOG(LWARNING, ("Can't register D-Bus service", kService.toStdString(), bus.lastError().message().toStdString()));
    return false;
  }
  return bus.registerObject(kPath, this, QDBusConnection::ExportScriptableSlots);
}

void UrlHandler::openUrl(QStringList const & urls)
{
  emit activated();
  for (auto const & url : urls)
  {
    // Files don't need the map.
    if (!m_bookmarksIO.OpenFile(url))
      m_pending.append(url);
  }
  ProcessPending();
}

void UrlHandler::ProcessPending()
{
  if (m_pending.isEmpty())
  {
    m_retryTimer.stop();
    return;
  }
  if (!m_framework.IsDrapeEngineCreated())
  {
    m_retryTimer.start();
    return;
  }
  m_retryTimer.stop();
  auto const pending = std::move(m_pending);
  m_pending.clear();
  for (auto const & url : pending)
    Process(url);
}

void UrlHandler::Process(QString const & url)
{
  using UrlType = url_scheme::ParsedMapApi::UrlType;
  // Request types as handled by Factory.UrlProcessor on Android.
  switch (m_framework.ParseAndSetApiURL(url.toStdString()))
  {
  case UrlType::Map: m_framework.ExecuteMapApiRequest(); break;
  case UrlType::Route:
  {
    auto const data = m_framework.GetParsedRoutingData();
    QVariantList points;
    for (auto const & point : data.m_points)
    {
      auto const ll = mercator::ToLatLon(point.m_org);
      points.append(QVariantMap{{"lat", ll.m_lat}, {"lon", ll.m_lon}, {"name", QString::fromStdString(point.m_name)}});
    }
    emit routeRequested(static_cast<int>(data.m_type), points);
    break;
  }
  case UrlType::Search:
  case UrlType::Crosshair:
  {
    if (auto const center = m_framework.GetParsedCenterLatLon(); center.IsValid())
    {
      m_framework.StopLocationFollow();
      m_framework.SetViewportCenter(mercator::FromLatLon(center), kLinkZoom);
    }
    auto const request = m_framework.GetParsedSearchRequest();
    if (!request.m_query.empty())
      emit searchRequested(QString::fromStdString(request.m_query));
    break;
  }
  default: LOG(LWARNING, ("Unsupported link", url.toStdString())); break;
  }
}
}  // namespace sailfish
