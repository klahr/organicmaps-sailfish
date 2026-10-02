#include "sailfish/routing.hpp"

#include "sailfish/app_info.hpp"
#include "sailfish/countries_model.hpp"

#include "map/framework.hpp"
#include "map/place_page_info.hpp"
#include "map/routing_manager.hpp"
#include "map/transit/transit_display.hpp"

#include "routing/following_info.hpp"
#include "routing/routing_callbacks.hpp"
#include "routing/routing_options.hpp"

#include "storage/storage.hpp"

#include <QColor>
#include <QVariantMap>

namespace sailfish
{
namespace
{
using routing::RouterResultCode;

// The step icons of TransitStepType on Android.
QString TransitIcon(TransitType type)
{
  switch (type)
  {
  case TransitType::IntermediatePoint:
  case TransitType::Pedestrian: return QStringLiteral("ic_20px_route_planning_walk.webp");
  case TransitType::Subway: return QStringLiteral("ic_20px_route_planning_metro.webp");
  case TransitType::Train: return QStringLiteral("ic_20px_route_planning_train.webp");
  case TransitType::Monorail: return QStringLiteral("ic_20px_route_planning_monorail.webp");
  case TransitType::Tram:
  case TransitType::CableTram: return QStringLiteral("ic_20px_route_planning_tram.svg");
  case TransitType::LightRail:
  case TransitType::AerialLift:
  case TransitType::Funicular: return QStringLiteral("ic_20px_route_planning_lightrail.webp");
  default: return QStringLiteral("ic_20px_route_planning_bus.svg");
  }
}

QString FormatTime(int seconds)
{
  // Like RoutingController.formatRoutingTime on Android.
  int const minutes = (seconds + 59) / 60;
  QString const min = QString::number(minutes % 60) + ' ' + Localized(QStringLiteral("minute"));
  if (minutes < 60)
    return min;
  QString const hours = QString::number(minutes / 60) + ' ' + Localized(QStringLiteral("hour"));
  return minutes % 60 ? hours + ' ' + min : hours;
}
}  // namespace

Routing::Routing(Framework & framework, QObject * parent) : QObject(parent), m_framework(framework)
{
  connect(this, &Routing::routeBuildingFinished, this, &Routing::OnRouteBuilt, Qt::QueuedConnection);
  m_framework.GetRoutingManager().SetRouteBuildingListener(
      [this](RouterResultCode code, storage::CountriesSet const & absent)
  {
    QStringList ids;
    for (auto const & id : absent)
      ids.append(QString::fromStdString(id));
    emit routeBuildingFinished(static_cast<int>(code), ids);
  });

  // Rebuild once the maps the route needs are downloaded, like on Android.
  m_storageSlot = m_framework.GetStorage().Subscribe([this](storage::CountryId const & id)
  {
    if (!m_missingMaps.contains(QString::fromStdString(id)))
      return;
    auto & storage = m_framework.GetStorage();
    for (auto const & missing : m_missingMaps)
    {
      storage::NodeStatuses statuses;
      storage.GetNodeStatuses(missing.toStdString(), statuses);
      if (statuses.m_status != storage::NodeStatus::OnDisk)
        return;
    }
    Build();
  }, [](storage::CountryId const &, downloader::Progress const &) {});
}

Routing::~Routing()
{
  m_framework.GetStorage().Unsubscribe(m_storageSlot);
  // RoutingManager requires a listener.
  m_framework.GetRoutingManager().SetRouteBuildingListener([](RouterResultCode, storage::CountriesSet const &) {});
}

bool Routing::active() const
{
  return m_framework.GetRoutingManager().GetRoutePointsCount() > 0;
}

int Routing::routerType() const
{
  return static_cast<int>(m_framework.GetRoutingManager().GetLastUsedRouter());
}

void Routing::setRouterType(int type)
{
  if (type == routerType())
    return;
  auto & manager = m_framework.GetRoutingManager();
  auto const router = static_cast<routing::RouterType>(type);
  manager.SetRouter(router);
  manager.SetLastUsedRouter(router);
  emit routerTypeChanged();
  Build();
}

QVariantList Routing::points() const
{
  QVariantList points;
  for (auto const & point : m_framework.GetRoutingManager().GetRoutePoints())
  {
    QVariantMap item;
    item["type"] = static_cast<int>(point.m_pointType);
    item["isMyPosition"] = point.m_isMyPosition;
    item["title"] =
        point.m_isMyPosition ? Localized(QStringLiteral("core_my_position")) : QString::fromStdString(point.m_title);
    item["subtitle"] = QString::fromStdString(point.m_subTitle);
    points.append(item);
  }
  return points;
}

QString Routing::missingMapsSize() const
{
  qint64 size = 0;
  for (auto const & id : m_missingMaps)
  {
    storage::NodeAttrs attrs;
    m_framework.GetStorage().GetNodeAttrs(id.toStdString(), attrs);
    size += static_cast<qint64>(attrs.m_mwmSize);
  }
  return CountriesModel::formatSize(size);
}

int Routing::avoidRoads() const
{
  return routing::RoutingOptions::LoadCarOptionsFromSettings().GetOptions();
}

void Routing::setAvoidRoads(int roads)
{
  routing::RoutingOptions::SaveCarOptionsToSettings(
      routing::RoutingOptions(static_cast<routing::RoutingOptions::RoadType>(roads)));
  emit optionsChanged();
  // The options only change car routes.
  if (routerType() == Vehicle)
    Build();
}

bool Routing::routeOptimization() const
{
  return routing::RoutingOptions::LoadRouteOptimizationFromSettings();
}

void Routing::setRouteOptimization(bool enabled)
{
  routing::RoutingOptions::SaveRouteOptimizationToSettings(enabled);
  emit optionsChanged();
}

void Routing::routeFromPlace()
{
  AddPlacePoint(Start);
}

void Routing::routeToPlace()
{
  AddPlacePoint(Finish);
}

void Routing::addStopFromPlace()
{
  AddPlacePoint(Intermediate);
}

void Routing::setStartToMyPosition()
{
  RouteMarkData start;
  start.m_pointType = RouteMarkType::Start;
  start.m_isMyPosition = true;
  if (auto const position = m_framework.GetCurrentPosition())
    start.m_position = *position;
  m_framework.GetRoutingManager().AddRoutePoint(std::move(start), false /* optimize */);
  OnPointsChanged();
}

void Routing::AddPlacePoint(int type)
{
  if (!m_framework.HasPlacePageInfo())
    return;

  auto const & info = m_framework.GetCurrentPlacePageInfo();
  RouteMarkData point;
  point.m_pointType = static_cast<RouteMarkType>(type);
  point.m_title = info.GetTitle();
  point.m_subTitle = info.GetSubtitle();
  point.m_isMyPosition = info.IsMyPosition();
  point.m_position = info.GetMercator();

  auto & manager = m_framework.GetRoutingManager();
  bool const optimize = type == Intermediate && routing::RoutingOptions::LoadRouteOptimizationFromSettings();
  manager.AddRoutePoint(std::move(point), optimize);

  // The route panel takes the place of the place page.
  m_framework.DeactivateMapSelection();

  // A route to a place starts from the current position unless a start was chosen, as on Android.
  bool hasStart = false;
  for (auto const & p : manager.GetRoutePoints())
    hasStart |= p.m_pointType == RouteMarkType::Start;
  if (!hasStart && type == Finish)
    setStartToMyPosition();
  else
    OnPointsChanged();
}

void Routing::removePoint(int index)
{
  auto const points = m_framework.GetRoutingManager().GetRoutePoints();
  if (index < 0 || index >= static_cast<int>(points.size()))
    return;
  auto const & point = points[static_cast<size_t>(index)];
  m_framework.GetRoutingManager().RemoveRoutePoint(point.m_pointType, point.m_intermediateIndex);
  OnPointsChanged();
}

void Routing::movePoint(int from, int to)
{
  auto & manager = m_framework.GetRoutingManager();
  auto const count = static_cast<int>(manager.GetRoutePointsCount());
  if (from < 0 || to < 0 || from >= count || to >= count || from == to)
    return;
  manager.MoveRoutePoint(static_cast<size_t>(from), static_cast<size_t>(to));
  OnPointsChanged();
}

void Routing::downloadMissingMaps()
{
  for (auto const & id : m_missingMaps)
    m_framework.GetStorage().DownloadNode(id.toStdString());
}

void Routing::close()
{
  m_framework.GetRoutingManager().CloseRouting(true /* removeRoutePoints */);
  m_building = m_built = false;
  m_summary.clear();
  m_walkingDistance.clear();
  m_transitSteps.clear();
  m_missingMaps.clear();
  SetError({}, {});
  emit pointsChanged();
}

void Routing::OnPointsChanged()
{
  emit pointsChanged();
  if (!active())
  {
    close();
    return;
  }
  Build();
}

void Routing::Build()
{
  auto & manager = m_framework.GetRoutingManager();
  m_built = false;
  m_summary.clear();
  m_walkingDistance.clear();
  m_transitSteps.clear();
  m_missingMaps.clear();
  SetError({}, {});
  if (manager.GetRoutePointsCount() < 2)
  {
    m_building = false;
    manager.RemoveRoute(false /* deactivateFollowing */);
    emit stateChanged();
    return;
  }
  manager.SetRouter(manager.GetLastUsedRouter());
  m_building = true;
  emit stateChanged();
  manager.BuildRoute();
}

void Routing::SetError(QString const & title, QString const & message)
{
  m_errorTitle = title;
  m_errorMessage = message;
}

void Routing::OnRouteBuilt(int code, QStringList const & absentCountries)
{
  auto const result = static_cast<RouterResultCode>(code);
  if (result == RouterResultCode::Cancelled)
    return;

  m_building = false;
  auto const & L = [](char const * key) { return Localized(QString::fromLatin1(key)); };
  switch (result)
  {
  case RouterResultCode::NoError:
  case RouterResultCode::HasWarnings:
  {
    m_built = true;
    routing::FollowingInfo info;
    m_framework.GetRoutingManager().GetRouteFollowingInfo(info);
    QString const distance = QString::fromStdString(info.m_distToTarget.ToString());
    if (routerType() == Transit)
    {
      // Total time, the walking distance and the legs, as RoutingBottomMenuController.showTransitInfo().
      auto const transit = m_framework.GetRoutingManager().GetTransitRouteInfo();
      m_summary = FormatTime(transit.m_totalTimeInSec);
      if (transit.m_totalPedestrianTimeInSec > 0)
      {
        m_walkingDistance =
            QString::fromStdString(transit.m_totalPedestrianDistanceStr + " " + transit.m_totalPedestrianUnitsSuffix);
      }
      for (auto const & step : transit.m_steps)
      {
        bool const walk = step.m_type == TransitType::Pedestrian || step.m_type == TransitType::IntermediatePoint;
        QVariantMap item;
        item["icon"] = TransitIcon(step.m_type);
        item["number"] = QString::fromStdString(step.m_number);
        item["color"] = walk ? QString() : QColor::fromRgba(step.m_colorARGB).name();
        m_transitSteps.append(item);
      }
    }
    else
    {
      m_summary = routerType() == Ruler ? L("placepage_distance") + ": " + distance
                                        : FormatTime(info.m_time) + QStringLiteral(" • ") + distance;
    }
    break;
  }
  // Messages of ResultCodesHelper on Android.
  case RouterResultCode::NoCurrentPosition:
    SetError(L("dialog_routing_location_turn_on"), L("dialog_routing_location_unknown_turn_on"));
    break;
  case RouterResultCode::StartPointNotFound:
    SetError(L("dialog_routing_change_start"), L("dialog_routing_start_not_determined"));
    break;
  case RouterResultCode::EndPointNotFound:
    SetError(L("dialog_routing_change_end"), L("dialog_routing_end_not_determined"));
    break;
  case RouterResultCode::IntermediatePointNotFound:
    SetError(L("dialog_routing_change_intermediate"), L("dialog_routing_intermediate_not_determined"));
    break;
  case RouterResultCode::PointsInDifferentMWM: SetError({}, L("routing_failed_cross_mwm_building")); break;
  case RouterResultCode::FileTooOld: SetError(L("downloader_update_maps"), L("downloader_mwm_migration_dialog")); break;
  case RouterResultCode::TransitRouteNotFoundNoNetwork: SetError({}, L("transit_not_found")); break;
  case RouterResultCode::TransitRouteNotFoundTooLongPedestrian:
    SetError(L("dialog_pedestrian_route_is_long_header"), L("dialog_pedestrian_route_is_long_message"));
    break;
  case RouterResultCode::NeedMoreMaps:
    m_missingMaps = absentCountries;
    SetError(L("dialog_routing_download_and_build_cross_route"), L("dialog_routing_download_cross_route"));
    break;
  case RouterResultCode::RouteNotFound:
  case RouterResultCode::RouteNotFoundRedressRouteError:
    m_missingMaps = absentCountries;
    if (m_missingMaps.isEmpty())
      SetError(L("dialog_routing_unable_locate_route"), L("dialog_routing_cant_build_route"));
    else
      SetError(L("routing_download_maps_along"), L("routing_requires_all_map"));
    break;
  default: SetError(L("dialog_routing_system_error"), L("dialog_routing_application_error")); break;
  }
  emit stateChanged();
}
}  // namespace sailfish
