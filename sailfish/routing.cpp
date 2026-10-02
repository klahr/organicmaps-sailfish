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
#include "routing/turns.hpp"

#include "indexer/map_style.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/settings.hpp"

#include "storage/storage.hpp"

#include <QColor>
#include <QDateTime>
#include <QLocale>
#include <QVariantMap>

#include <utility>

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

// The turn arrows of CarDirection and PedestrianTurnDirection on Android.
QString TurnIcon(routing::turns::CarDirection turn, uint32_t exitNum)
{
  using routing::turns::CarDirection;
  switch (turn)
  {
  case CarDirection::TurnRight: return QStringLiteral("ic_turn_right.webp");
  case CarDirection::TurnSharpRight: return QStringLiteral("ic_turn_right_sharp.webp");
  case CarDirection::TurnSlightRight: return QStringLiteral("ic_turn_right_slight.webp");
  case CarDirection::TurnLeft: return QStringLiteral("ic_turn_left.webp");
  case CarDirection::TurnSharpLeft: return QStringLiteral("ic_turn_left_sharp.webp");
  case CarDirection::TurnSlightLeft: return QStringLiteral("ic_turn_left_slight.webp");
  case CarDirection::UTurnLeft: return QStringLiteral("ic_turn_uleft.webp");
  case CarDirection::UTurnRight: return QStringLiteral("ic_turn_uright.webp");
  case CarDirection::EnterRoundAbout:
  case CarDirection::LeaveRoundAbout:
  case CarDirection::StayOnRoundAbout:
    return exitNum >= 1 && exitNum <= 12 ? QStringLiteral("ic_roundabout_exit_%1.svg").arg(exitNum)
                                         : QStringLiteral("ic_turn_round.svg");
  case CarDirection::ReachedYourDestination: return QStringLiteral("ic_turn_finish.webp");
  case CarDirection::ExitHighwayToLeft: return QStringLiteral("ic_exit_highway_to_left.webp");
  case CarDirection::ExitHighwayToRight: return QStringLiteral("ic_exit_highway_to_right.webp");
  default: return QStringLiteral("ic_turn_straight.webp");
  }
}

QString TurnIcon(routing::turns::PedestrianDirection turn)
{
  using routing::turns::PedestrianDirection;
  switch (turn)
  {
  case PedestrianDirection::TurnRight: return QStringLiteral("ic_turn_right.webp");
  case PedestrianDirection::TurnLeft: return QStringLiteral("ic_turn_left.webp");
  case PedestrianDirection::ReachedYourDestination: return QStringLiteral("ic_turn_finish.webp");
  default: return QStringLiteral("ic_turn_straight.webp");
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

bool Routing::canStart() const
{
  auto const type = routerType();
  return m_built && type != Ruler && type != Transit;
}

void Routing::start()
{
  if (!canStart())
    return;
  auto & manager = m_framework.GetRoutingManager();
  // Navigation follows the position; Android asks to move the start there first.
  auto const points = manager.GetRoutePoints();
  if (!points.empty() && !points.front().m_isMyPosition)
  {
    m_startWhenBuilt = true;
    setStartToMyPosition();
    return;
  }
  manager.FollowRoute();
  m_navigating = true;
  SetNavigationStyle(true);
  UpdateNavigation(-1.0);
}

void Routing::stopNavigation()
{
  m_navigating = false;
  m_navigation.clear();
  SetNavigationStyle(false);
  close();
  emit navigationChanged();
}

void Routing::SetNavigationStyle(bool enabled)
{
  // Car navigation uses the vehicle map style, like ThemeSwitcher on Android.
  MapStyle const style = m_framework.GetMapStyle();
  bool const dark = MapStyleIsDark(style);
  if (enabled && routerType() == Vehicle)
    m_framework.SetMapStyle(dark ? MapStyleVehicleDark : MapStyleVehicleLight);
  else if (!enabled && (style == MapStyleVehicleDark || style == MapStyleVehicleLight))
  {
    bool const outdoors = Framework::LoadOutdoorsEnabled();
    m_framework.SetMapStyle(dark ? (outdoors ? MapStyleOutdoorsDark : MapStyleDefaultDark)
                                 : (outdoors ? MapStyleOutdoorsLight : MapStyleDefaultLight));
  }
}

void Routing::UpdateNavigation(double speedMps)
{
  if (!m_navigating)
    return;
  auto & manager = m_framework.GetRoutingManager();
  if (manager.IsRouteFinished())
  {
    stopNavigation();
    return;
  }

  routing::FollowingInfo info;
  manager.GetRouteFollowingInfo(info);
  if (!info.IsValid())
    return;

  measurement_utils::Units units = measurement_utils::Units::Metric;
  settings::TryGet(settings::kMeasurementUnits, units);
  bool const pedestrian = routerType() == Pedestrian;

  QVariantMap nav;
  nav["turnIcon"] = pedestrian ? TurnIcon(info.m_pedestrianTurn) : TurnIcon(info.m_turn, info.m_exitNum);
  nav["distanceToTurn"] = QString::fromStdString(info.m_distToTurn.ToString());
  nav["street"] = QString::fromStdString(info.m_nextStreetName);
  nav["nextTurnIcon"] =
      !pedestrian && info.m_nextTurn != routing::turns::CarDirection::None ? TurnIcon(info.m_nextTurn, 0) : QString();
  nav["timeLeft"] = FormatTime(info.m_time);
  nav["distanceLeft"] = QString::fromStdString(info.m_distToTarget.ToString());
  // Number and units apart, as the Android bottom sheet shows them.
  nav["distanceLeftValue"] = QString::fromStdString(info.m_distToTarget.GetDistanceString());
  nav["distanceLeftUnits"] = QString::fromStdString(info.m_distToTarget.GetUnitsString());
  int const minutes = (info.m_time + 59) / 60;
  nav["hoursLeft"] = minutes / 60;
  nav["minutesLeft"] = minutes % 60;
  nav["hourUnits"] = Localized(QStringLiteral("hour"));
  nav["minuteUnits"] = Localized(QStringLiteral("minute"));
  nav["arrival"] = QLocale::system().toString(QTime::currentTime().addSecs(info.m_time), QLocale::ShortFormat);
  nav["speed"] = speedMps >= 0 ? QString::fromStdString(measurement_utils::FormatSpeedNumeric(speedMps, units))
                               : QStringLiteral("0");
  nav["speedUnits"] = Localized(units == measurement_utils::Units::Imperial ? QStringLiteral("miles_per_hour")
                                                                            : QStringLiteral("kilometers_per_hour"));
  nav["speedLimit"] = info.m_speedLimitMps > 0
                        ? QString::fromStdString(measurement_utils::FormatSpeedNumeric(info.m_speedLimitMps, units))
                        : QString();
  nav["progress"] = info.m_completionPercent / 100.0;
  m_navigation = nav;
  emit navigationChanged();
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
  m_startWhenBuilt = false;
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
  if (std::exchange(m_startWhenBuilt, false) && m_built)
    start();
}
}  // namespace sailfish
