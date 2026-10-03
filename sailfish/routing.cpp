#include "sailfish/routing.hpp"

#include "sailfish/app_info.hpp"
#include "sailfish/countries_model.hpp"
#include "sailfish/voice_guide.hpp"

#include "map/framework.hpp"
#include "map/place_page_info.hpp"
#include "map/routing_manager.hpp"
#include "map/transit/transit_display.hpp"

#include "routing/following_info.hpp"
#include "routing/routing_callbacks.hpp"
#include "routing/routing_options.hpp"
#include "routing/turns.hpp"

#include "indexer/map_style.hpp"

#include "platform/get_text_by_id.hpp"
#include "platform/languages.hpp"
#include "platform/measurement_utils.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"

#include "storage/storage.hpp"

#include <QColor>
#include <QDateTime>
#include <QLocale>
#include <QVariantMap>

#include <algorithm>
#include <utility>
#include <vector>

namespace sailfish
{
namespace
{
using routing::RouterResultCode;

char const kVoiceEnabledSetting[] = "SailfishVoiceInstructions";
// Empty for the app language.
char const kVoiceLanguageSetting[] = "SailfishVoiceLanguage";
char const kAnnounceStreetsSetting[] = "SailfishVoiceStreetNames";

QString VoiceLanguageName(std::string const & code)
{
  for (auto const & [lang, name] : routing::turns::sound::kLanguageList)
    if (lang == code)
      return QString::fromUtf8(name.data(), static_cast<int>(name.size()));
  return QString::fromStdString(code);
}

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

Routing::Routing(Framework & framework, QObject * parent)
  : QObject(parent)
  , m_framework(framework)
  , m_voice(new VoiceGuide(this))
{
  SetupVoice();
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
  // Picks up voices installed since the start.
  m_voice->Refresh();
  m_framework.GetRoutingManager().EnableTurnNotifications(voiceEnabled());
  SetNavigationStyle(true);
  UpdateNavigation(-1.0);
}

void Routing::stopNavigation()
{
  m_voice->Stop();
  m_navigating = false;
  m_navigation.clear();
  SetNavigationStyle(false);
  close();
  emit navigationChanged();
}

std::string Routing::AppVoiceLanguage() const
{
  // The app language when there are voice instructions in it, like the system language on Android.
  auto const has = [](std::string const & code)
  {
    auto const & list = routing::turns::sound::kLanguageList;
    return std::any_of(list.begin(), list.end(), [&code](auto const & lang) { return lang.first == code; });
  };
  auto language = languages::GetCurrentTwine();
  if (!has(language))
    language = language.substr(0, language.find('-'));
  return has(language) ? language : "en";
}

void Routing::SetupVoice()
{
  // Voices are found in the background, Speech Note ones over D-Bus.
  connect(m_voice, &VoiceGuide::Changed, this, [this]
  {
    auto & manager = m_framework.GetRoutingManager();
    if (m_voice->IsAvailable())
      manager.SetTurnNotificationsLocale(m_voice->Language());
    if (m_navigating)
      manager.EnableTurnNotifications(voiceEnabled());
    emit voiceChanged();
  });
  std::string preferred;
  settings::TryGet(kVoiceLanguageSetting, preferred);
  m_voice->SetPreferredLanguage(preferred, AppVoiceLanguage());
  m_voice->Refresh();
}

bool Routing::voiceAvailable() const
{
  return m_voice->IsAvailable();
}

bool Routing::voiceEnabled() const
{
  bool enabled = true;
  settings::TryGet(kVoiceEnabledSetting, enabled);
  return enabled && m_voice->IsAvailable();
}

void Routing::setVoiceEnabled(bool enabled)
{
  settings::Set(kVoiceEnabledSetting, enabled);
  m_framework.GetRoutingManager().EnableTurnNotifications(voiceEnabled());
  if (!enabled)
    m_voice->Stop();
  emit voiceChanged();
}

QString Routing::voiceLanguage() const
{
  return QString::fromStdString(m_voice->Language());
}

void Routing::setVoiceLanguage(QString const & language)
{
  // The app language again is stored as no choice, so that it follows later app language changes.
  auto const code = language.toStdString();
  settings::Set(kVoiceLanguageSetting, code == AppVoiceLanguage() ? std::string() : code);
  m_voice->SetPreferredLanguage(code, AppVoiceLanguage());
}

QVariantList Routing::voiceLanguages() const
{
  QVariantList languages;
  for (auto const & code : m_voice->Languages())
  {
    languages.append(QVariantMap{{"code", QString::fromStdString(code)},
                                 {"name", VoiceLanguageName(code)},
                                 {"speechNote", m_voice->HasSpeechNoteVoice(code)}});
  }
  return languages;
}

bool Routing::speechNoteInstalled() const
{
  return m_voice->IsSpeechNoteInstalled();
}

bool Routing::speechNoteVoice() const
{
  return m_voice->IsAvailable() && m_voice->HasSpeechNoteVoice(m_voice->Language());
}

QString Routing::wantedVoiceLanguageName() const
{
  std::string preferred;
  settings::TryGet(kVoiceLanguageSetting, preferred);
  return VoiceLanguageName(preferred.empty() ? AppVoiceLanguage() : preferred);
}

bool Routing::announceStreets() const
{
  bool announce = false;
  settings::TryGet(kAnnounceStreetsSetting, announce);
  return announce;
}

void Routing::setAnnounceStreets(bool announce)
{
  settings::Set(kAnnounceStreetsSetting, announce);
  emit voiceChanged();
}

void Routing::refreshVoice()
{
  m_voice->Refresh();
}

void Routing::openSpeechNote()
{
  m_voice->OpenSpeechNote();
}

void Routing::testVoice()
{
  // Turn notifications in the voice language. Android reads app tips instead, which here would be in the app
  // language when no voice speaks it.
  std::pair<char const *, char const *> constexpr kTests[] = {{"in_200_meters", "make_a_left_turn"},
                                                              {"in_1_kilometer", "make_a_slight_right_turn"},
                                                              {nullptr, "you_have_reached_the_destination"}};
  if (!m_voice->IsAvailable())
    return;
  auto const text = platform::GetTextByIdFactory(platform::TextSource::TtsSound, m_voice->Language());
  if (!text)
    return;
  auto const & [distance, turn] = kTests[m_voiceTestIndex];
  QString speech = QString::fromStdString((*text)(turn));
  if (distance)
    speech = QString::fromStdString((*text)(distance)) + ", " + speech.left(1).toLower() + speech.mid(1);
  m_voice->Speak({speech});
  m_voiceTestIndex = (m_voiceTestIndex + 1) % std::size(kTests);
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
  // Before the finish check, so that the arrival is announced, like on Android.
  if (voiceEnabled())
  {
    std::vector<std::string> notifications;
    manager.GenerateNotifications(notifications, announceStreets());
    QStringList texts;
    for (auto const & text : notifications)
      texts.append(QString::fromStdString(text));
    m_voice->Speak(texts);
  }
  if (manager.IsRouteFinished())
  {
    // Lets the arrival be heard.
    m_navigating = false;
    m_navigation.clear();
    SetNavigationStyle(false);
    close();
    emit navigationChanged();
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
