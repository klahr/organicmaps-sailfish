#include "sailfish/place_page.hpp"

#include "sailfish/app_info.hpp"

#include "map/bookmark_helpers.hpp"
#include "map/bookmark_manager.hpp"
#include "map/elevation_info.hpp"
#include "map/framework.hpp"
#include "map/place_page_info.hpp"
#include "map/track.hpp"

#include "ge0/url_generator.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/validate_and_format_contacts.hpp"

#include "editor/opening_hours_ui.hpp"
#include "editor/ui2oh.hpp"

#include "opening_hours/opening_hours.hpp"

#include "platform/distance.hpp"
#include "platform/localization.hpp"
#include "platform/settings.hpp"

#include "geometry/angles.hpp"
#include "geometry/mercator.hpp"

#include "base/math.hpp"

#include <QColor>
#include <QDateTime>
#include <QLocale>
#include <QRegularExpression>
#include <QUrl>
#include <QVariantMap>

#include <cmath>

namespace sailfish
{
namespace
{
using feature::Metadata;

// Same setting and default as the desktop place page.
std::string_view constexpr kCoordinatesFormatSetting = "CoordinatesFormat";

// Enough points for a phone wide chart.
size_t constexpr kMaxProfilePoints = 600;

QString FormatHourMinutes(osmoh::HourMinutes const & hm)
{
  // Noon and midnight by name on a 12-hour clock, like Timetable.formatOpenShifts() on Android: 12:00 AM and PM
  // are often misread. A 24-hour clock has no such doubt.
  auto const hours = hm.GetHoursCount();
  if (hm.GetMinutesCount() == 0 && !Is24HourClock())
  {
    if (hours == 12)
      return Localized("noon");
    if (hours == 0 || hours == 24)
      return Localized("midnight");
  }
  // 24:00 closes at the end of the day; QTime can't hold it.
  if (hours == 24 && hm.GetMinutesCount() == 0)
    return QStringLiteral("24:00");
  return FormatTime(QTime(static_cast<int>(hours % 24), static_cast<int>(hm.GetMinutesCount())));
}

// Opening shifts of a day: the opening time without the closed times, one per line.
QString FormatShifts(editor::ui::TimeTable const & tt)
{
  QStringList shifts;
  auto const add = [&shifts](osmoh::HourMinutes const & start, osmoh::HourMinutes const & end)
  {
    // A start after the end is an overnight shift; only empty ones are dropped.
    if (start.GetDurationCount() != end.GetDurationCount())
      shifts.append(FormatHourMinutes(start) + "—" + FormatHourMinutes(end));
  };
  auto start = tt.GetOpeningTime().GetStart().GetHourMinutes();
  for (auto const & closed : tt.GetExcludeTime())
  {
    add(start, closed.GetStart().GetHourMinutes());
    start = closed.GetEnd().GetHourMinutes();
  }
  add(start, tt.GetOpeningTime().GetEnd().GetHourMinutes());
  return shifts.join('\n');
}

// The week from today, days with the same hours together, like WeekScheduleBuilder on Android.
QVariantList MakeWeekSchedule(editor::ui::TimeTableSet & tts)
{
  std::vector<editor::ui::TimeTable> tables;
  for (size_t i = 0; i < tts.Size(); ++i)
    tables.push_back(tts.Get(i));
  // osmoh::Weekday and the Android Calendar count from Sunday = 1, Qt from Monday = 1.
  int const today = QDate::currentDate().dayOfWeek() % 7 + 1;
  std::vector<int> week;
  for (int i = 0; i < 7; ++i)
    week.push_back((today - 1 + i) % 7 + 1);
  auto const find = [&tables](int day) -> editor::ui::TimeTable const *
  {
    for (auto const & tt : tables)
      if (tt.GetOpeningDays().count(static_cast<osmoh::Weekday>(day)))
        return &tt;
    return nullptr;
  };
  auto const dayName = [](int day) { return QLocale::system().dayName(day == 1 ? 7 : day - 1, QLocale::ShortFormat); };

  QVariantList schedule;
  for (size_t i = 0; i < week.size(); ++i)
  {
    size_t const first = i;
    auto const * tt = find(week[i]);
    if (tt)
    {
      while (i + 1 < week.size() && tt->GetOpeningDays().count(static_cast<osmoh::Weekday>(week[i + 1])))
        ++i;
    }
    else
    {
      // Closed days run until the next open one.
      while (i + 1 < week.size() && !find(week[i + 1]))
        ++i;
    }
    QString days = dayName(week[first]);
    if (i != first)
      days += "-" + dayName(week[i]);
    QString hours = !tt                     ? Localized("day_off")
                  : tt->IsTwentyFourHours() ? Localized("editor_time_allday")
                                            : FormatShifts(*tt);
    if (hours.isEmpty())
      hours = Localized("day_off");
    // The week starts today, so only the first row holds it.
    schedule.append(QVariantMap{{"days", days}, {"hours", hours}, {"today", first == 0}});
  }
  return schedule;
}

int32_t SavedCoordinatesFormat()
{
  auto saved = static_cast<int32_t>(place_page::CoordinatesFormat::LatLonDecimal);
  settings::TryGet(kCoordinatesFormatSetting, saved);
  return saved;
}
}  // namespace

PlacePage::PlacePage(Framework & framework, QObject * parent) : QObject(parent), m_framework(framework)
{
  m_framework.SetPlacePageListeners([this] { Update(); }, [this]
  {
    m_open = false;
    emit changed();
  }, [this] { Update(); }, {} /* onSwitchFullScreen */);
  auto & manager = m_framework.GetBookmarkManager();
  manager.SetElevationActivePointChangedCallback([this](kml::TrackId, double) { emit elevationPointsChanged(); });
  manager.SetElevationMyPositionChangedCallback([this](kml::TrackId, double) { emit elevationPointsChanged(); });
}

PlacePage::~PlacePage()
{
  m_framework.SetPlacePageListeners({}, {}, {}, {});
  auto & manager = m_framework.GetBookmarkManager();
  manager.SetElevationActivePointChangedCallback({});
  manager.SetElevationMyPositionChangedCallback({});
}

double PlacePage::elevationActivePoint() const
{
  auto const & manager = m_framework.GetBookmarkManager();
  return m_isTrack && manager.HasTrack(m_userMarkId) ? manager.GetElevationActivePoint(m_userMarkId) : -1.0;
}

double PlacePage::elevationMyPosition() const
{
  auto const & manager = m_framework.GetBookmarkManager();
  return m_isTrack && manager.HasTrack(m_userMarkId) ? manager.GetElevationMyPosition(m_userMarkId) : -1.0;
}

void PlacePage::setElevationActivePoint(double distance)
{
  if (m_isTrack && m_framework.GetBookmarkManager().HasTrack(m_userMarkId))
    m_framework.GetBookmarkManager().SetElevationActivePoint(m_userMarkId, distance);
}

void PlacePage::close()
{
  m_framework.DeactivateMapSelection();
}

void PlacePage::nextCoordinatesFormat()
{
  if (!m_framework.HasPlacePageInfo())
    return;
  auto const & info = m_framework.GetCurrentPlacePageInfo();
  auto const entries = place_page::GetAvailableCoordinateFormats(info.GetLatLon(), info.GetCountryId());
  settings::Set(kCoordinatesFormatSetting,
                static_cast<int32_t>(place_page::NextCoordinateFormat(entries, SavedCoordinatesFormat())));
  Update();
}

void PlacePage::toggleBookmark()
{
  if (!m_framework.HasPlacePageInfo())
    return;

  // Same steps as the Android BookmarkManager and Framework JNI. Reselecting the place updates the page.
  auto const & info = m_framework.GetCurrentPlacePageInfo();
  auto & manager = m_framework.GetBookmarkManager();
  auto buildInfo = info.GetBuildInfo();
  if (info.IsBookmark())
  {
    manager.GetEditSession().DeleteBookmark(info.GetBookmarkId());
    buildInfo.m_match = place_page::BuildInfo::Match::FeatureOnly;
    buildInfo.m_userMarkId = kml::kInvalidMarkId;
    buildInfo.m_source = place_page::BuildInfo::Source::Other;
  }
  else
  {
    kml::BookmarkData data;
    data.m_name = info.FormatNewBookmarkName();
    data.m_point = info.GetMercator();
    data.m_color = m_framework.LastEditedBMColor();
    if (info.IsFeature())
      SaveFeatureTypes(info.GetTypes(), data);
    auto const * bookmark =
        manager.GetEditSession().CreateBookmark(std::move(data), m_framework.LastEditedBMCategory());
    buildInfo.m_match = place_page::BuildInfo::Match::Everything;
    buildInfo.m_userMarkId = bookmark->GetId();
  }
  m_framework.UpdatePlacePageInfoForCurrentSelection(buildInfo);
}

void PlacePage::Update()
{
  if (!m_framework.HasPlacePageInfo())
    return;

  auto const & info = m_framework.GetCurrentPlacePageInfo();
  m_open = true;
  m_title = QString::fromStdString(info.GetTitle());
  m_subtitle = QString::fromStdString(info.GetSubtitle());
  m_address = QString::fromStdString(info.GetSecondarySubtitle());

  auto const entries = place_page::GetAvailableCoordinateFormats(info.GetLatLon(), info.GetCountryId());
  auto const format = place_page::EffectiveCoordinateFormat(entries, SavedCoordinatesFormat());
  m_coordinateValues.clear();
  for (auto const & entry : entries)
  {
    if (entry.m_format == format)
      m_coordinates = QString::fromStdString(entry.m_display);
    m_coordinateValues.append(QString::fromStdString(entry.m_value));
  }

  m_isBookmark = info.IsBookmark();
  m_isTrack = info.IsTrack();
  m_userMarkId = m_isTrack ? info.GetTrackId() : info.GetBookmarkId();
  m_canEdit = info.ShouldShowEditPlace();
  m_canAddPlace = info.ShouldShowAddPlace();
  m_canAddBusiness = info.ShouldShowAddBusiness();
  m_editable = info.CanEditPlace();
  m_shareText = QString::fromStdString(m_framework.GetShareData(info).m_text);
  m_geoUri = QString::fromStdString(
      ge0::GenerateGeoUri(info.GetLatLon().m_lat, info.GetLatLon().m_lon, m_framework.GetDrawScale(), info.GetTitle()));
  UpdateOpeningHours(info.GetOpeningHours());
  m_wikiDescription = QString::fromStdString(info.GetWikiDescription());
  auto const wikipedia = info.GetMetadata(Metadata::FMD_WIKIPEDIA);
  m_wikiUrl = wikipedia.empty() ? QString() : QString::fromStdString(Metadata::ToWikiURL(std::string(wikipedia)));

  m_details.clear();
  auto const add = [this](QString const & icon, QString const & text, QString const & url = {})
  {
    if (!text.isEmpty())
      m_details.append(QVariantMap{{"icon", icon}, {"text", text}, {"url", url}});
  };
  // The order of place_page_details.xml and place_page_links_fragment.xml on Android.
  add("../../icons/placepage/ic_cuisine.svg", QString::fromStdString(info.FormatCuisines()));
  add("../../icons/placepage/ic_entrance.webp", ToQString(info.GetMetadata(Metadata::FMD_FLATS)));
  add("image://theme/icon-m-person", ToQString(info.GetMetadata(Metadata::FMD_OPERATOR)));
  if (auto const network = info.GetMetadata(Metadata::FMD_NETWORK); !network.empty())
    add("../../icons/placepage/ic_network_white.svg", Localized("network", {ToQString(network)}));

  // Websites show without the scheme and the trailing slash, like MapObject.getWebsiteUrl() on Android.
  auto const addWebsite = [&](char const * icon, Metadata::EType type, QString const & text = {})
  {
    QString const url = QUrl::fromPercentEncoding(ToQString(info.GetMetadata(type)).toUtf8());
    if (url.isEmpty())
      return;
    QString shown = url;
    shown.remove(QRegularExpression(QStringLiteral("^https?://"))).remove(QRegularExpression(QStringLiteral("/$")));
    add(icon, text.isEmpty() ? shown : text, url.contains("://") ? url : "https://" + url);
  };
  addWebsite("image://theme/icon-m-website", Metadata::FMD_WEBSITE);
  addWebsite("image://theme/icon-m-website", Metadata::FMD_HERITAGE_WEBSITE);
  addWebsite("../../icons/editor/ic_website_menu.svg", Metadata::FMD_WEBSITE_MENU, Localized("view_menu"));
  // Multiple phone numbers are separated by semicolons.
  for (auto const & phone : ToQString(info.GetMetadata(Metadata::FMD_PHONE_NUMBER)).split(';', QString::SkipEmptyParts))
    add("image://theme/icon-m-phone", phone.trimmed(), "tel:" + phone.trimmed());
  if (auto const email = ToQString(info.GetMetadata(Metadata::FMD_EMAIL)); !email.isEmpty())
    add("image://theme/icon-m-mail", email, "mailto:" + email);
  // Social networks show the contact as tagged and open its page.
  std::pair<Metadata::EType, char const *> constexpr kSocial[] = {{Metadata::FMD_CONTACT_FACEBOOK, "ic_facebook.svg"},
                                                                  {Metadata::FMD_CONTACT_INSTAGRAM, "ic_instagram.svg"},
                                                                  {Metadata::FMD_CONTACT_TWITTER, "ic_twitterx.svg"},
                                                                  {Metadata::FMD_CONTACT_VK, "ic_vk.svg"},
                                                                  {Metadata::FMD_CONTACT_LINE, "ic_line.svg"}};
  for (auto const & [type, icon] : kSocial)
  {
    auto const value = info.GetMetadata(type);
    if (!value.empty())
      add(QStringLiteral("../../icons/editor/%1").arg(icon), ToQString(value),
          QString::fromStdString(osm::socialContactToURL(type, value)));
  }
  if (auto const commons = info.GetMetadata(Metadata::FMD_WIKIMEDIA_COMMONS); !commons.empty())
  {
    add("../../icons/placepage/ic_wikimedia_commons.svg", Localized("wikimedia_commons"),
        QString::fromStdString(Metadata::ToWikimediaCommonsURL(std::string(commons))));
  }

  add("image://theme/icon-m-levels", ToQString(info.GetMetadata(Metadata::FMD_LEVEL)));
  if (auto const capacity = info.GetMetadata(Metadata::FMD_CAPACITY); !capacity.empty())
    add("../../icons/placepage/ic_capacity_white.svg", Localized("capacity", {ToQString(capacity)}));
  // Classificator types like wheelchair-yes and self_service-yes, localized like other types.
  if (auto const wheelchair = info.GetMetadata(Metadata::FMD_WHEELCHAIR); !wheelchair.empty())
    add("../../icons/placepage/ic_wheelchair_white.svg",
        QString::fromStdString(platform::GetLocalizedTypeName(std::string(wheelchair))));
  // Internet access as Yes or No, like on Android.
  if (auto const internet = info.GetInternet(); internet != feature::Internet::Unknown)
    add("image://theme/icon-m-wlan", Localized(internet == feature::Internet::No ? "no_available" : "yes_available"));
  if (info.GetMetadata(Metadata::FMD_DRIVE_THROUGH) == "yes")
    add("../../icons/placepage/ic_drive_through_white.svg", Localized("drive_through"));
  if (auto const selfService = info.GetMetadata(Metadata::FMD_SELF_SERVICE); !selfService.empty())
    add("../../icons/editor/ic_self_service.svg",
        QString::fromStdString(platform::GetLocalizedTypeName("self_service-" + std::string(selfService))));
  if (info.GetMetadata(Metadata::FMD_OUTDOOR_SEATING) == "yes")
    add("../../icons/placepage/ic_outdoor_seating.svg", Localized("outdoor_seating"));

  m_routes.clear();
  m_routeIds.clear();
  for (auto const & route : info.GetRoutes())
  {
    QString label = QString::fromStdString(route.m_ref);
    if (!route.m_from.empty() || !route.m_to.empty())
    {
      label += ": " + QString::fromStdString(route.m_from);
      if (!route.m_to.empty())
        label += " → " + QString::fromStdString(route.m_to);
    }
    uint32_t const argb = route.m_color.GetARGB();
    // No alpha means the relation has no color.
    QString const color = (argb >> 24) == 0 ? QString() : QColor::fromRgba(argb).name();
    m_routes.append(QVariantMap{{"label", label}, {"color", color}});
    m_routeIds.push_back(route.m_relID);
  }
  static uint32_t const tramStop = classif().GetTypeByPath({"railway", "tram_stop"});
  m_isTramStop = info.GetTypes().Has(tramStop);
  UpdateRouteRefs();

  UpdateTrack();
  UpdateDistance();
  emit changed();
}

void PlacePage::UpdateRouteRefs()
{
  // Each ref once: directions of a line repeat it, like formatRouteRefs() on Android.
  QStringList seen;
  QStringList refs;
  QString const active = QString::fromStdString(m_framework.GetActiveTransitRouteRef());
  if (m_framework.HasPlacePageInfo())
  {
    for (auto const & route : m_framework.GetCurrentPlacePageInfo().GetRoutes())
    {
      QString const ref = QString::fromStdString(route.m_ref);
      if (seen.contains(ref))
        continue;
      seen.append(ref);
      refs.append(ref == active ? "<b><u>" + ref.toHtmlEscaped() + "</u></b>" : ref.toHtmlEscaped());
    }
  }
  m_routeRefs = refs.join(QStringLiteral(" • "));
}

void PlacePage::showRoute(int index)
{
  if (index < 0 || index >= static_cast<int>(m_routeIds.size()))
    return;
  m_framework.ShowRouteTransit(m_routeIds[static_cast<size_t>(index)]);
  UpdateRouteRefs();
  emit changed();
}

void PlacePage::UpdateTrack()
{
  m_trackStats.clear();
  m_elevationProfile.clear();
  m_trackLength = 0;
  m_minElevation.clear();
  m_maxElevation.clear();
  auto & manager = m_framework.GetBookmarkManager();
  auto const * track = m_isTrack ? manager.GetTrack(m_userMarkId) : nullptr;
  if (!track)
    return;

  auto const add = [this](char const * label, std::string const & value)
  { m_trackStats.append(QVariantMap{{"label", Localized(label)}, {"value", QString::fromStdString(value)}}); };
  auto const stats = track->GetStatistics();
  m_trackLength = stats.m_length;
  add("elevation_profile_distance", stats.GetFormattedLength());
  if (stats.m_duration > 0)
    m_trackStats.append(QVariantMap{{"label", Localized("elevation_profile_time")},
                                    {"value", FormatDuration(static_cast<long>(stats.m_duration))}});

  auto const * elevation = track->GetElevationInfo();
  if (!elevation || elevation->IsEmpty())
    return;
  add("elevation_profile_ascent", stats.GetFormattedAscent());
  add("elevation_profile_descent", stats.GetFormattedDescent());
  add("elevation_profile_max_elevation", stats.GetFormattedMaxElevation());
  add("elevation_profile_min_elevation", stats.GetFormattedMinElevation());
  char const * const difficulties[] = {nullptr, "elevation_profile_diff_level_easy",
                                       "elevation_profile_diff_level_moderate", "elevation_profile_diff_level_hard"};
  if (auto const difficulty = elevation->GetDifficulty(); difficulty > 0 && difficulty < std::size(difficulties))
    add("elevation_profile_difficulty", Localized(difficulties[difficulty]).toStdString());
  m_minElevation = QString::fromStdString(stats.GetFormattedMinElevation());
  m_maxElevation = QString::fromStdString(stats.GetFormattedMaxElevation());

  size_t const count = elevation->GetSize();
  size_t const step = count / kMaxProfilePoints + 1;
  size_t i = 0;
  elevation->ForEachPoint([&](double distance, geometry::Altitude altitude)
  {
    // Keeps the last point, so that the profile ends with the track.
    if (i % step == 0 || i + 1 == count)
      m_elevationProfile << distance << altitude;
    ++i;
    m_trackLength = distance;
  });
  // The position marker follows location updates along the track.
  manager.UpdateElevationMyPosition(m_userMarkId);
}

void PlacePage::UpdateOpeningHours(std::string_view openingHours)
{
  m_openingHours = ToQString(openingHours);
  m_openingSchedule.clear();
  m_openState = OpenUnknown;
  m_openTitle.clear();
  m_openDescription.clear();

  osmoh::OpeningHours const oh(openingHours);
  if (openingHours.empty() || !oh.IsValid())
    return;

  // The weekly table, except for 24/7, like on Android.
  editor::ui::TimeTableSet tts;
  if (!oh.IsTwentyFourHours() && editor::MakeTimeTableSet(oh, tts))
    m_openingSchedule = MakeWeekSchedule(tts);

  // Local time without the feature time zone, like on Android.
  time_t const now = std::time(nullptr);
  auto const info = oh.GetInfo(now);
  if (info.state == osmoh::RuleState::Unknown)
    return;

  QLocale const locale = QLocale::system();
  auto const time = [](time_t t) { return FormatTime(QDateTime::fromTime_t(static_cast<uint>(t)).time()); };

  if (oh.IsTwentyFourHours())
  {
    m_openState = Open;
    m_openTitle = Localized("twentyfour_seven");
    return;
  }

  // Thresholds and wording follow PlacePageOpeningHoursFragment on Android.
  if (info.state == osmoh::RuleState::Open)
  {
    m_openState = Open;
    m_openTitle = Localized("editor_time_open");
    long const minutes = (info.nextTimeClosed - now) / 60;
    if (info.nextTimeClosed <= now)
      return;
    if (minutes < 3 * 60)
      m_openDescription =
          Localized("closes_in", {FormatDuration(info.nextTimeClosed - now)}) + " • " + time(info.nextTimeClosed);
    else if (minutes < 24 * 60)
      m_openDescription = Localized("closes_at", {time(info.nextTimeClosed)});
    return;
  }

  m_openState = Closed;
  m_openTitle = Localized("closed_now");
  if (info.nextTimeOpen <= now)
    return;
  long const minutes = (info.nextTimeOpen - now) / 60;
  QDateTime const opens = QDateTime::fromTime_t(static_cast<uint>(info.nextTimeOpen));
  if (minutes < 3 * 60)
    m_openDescription =
        Localized("opens_in", {FormatDuration(info.nextTimeOpen - now)}) + " • " + time(info.nextTimeOpen);
  else if (opens.date() == QDate::currentDate())
    m_openDescription = Localized("opens_at", {time(info.nextTimeOpen)});
  else if (minutes < 24 * 60)
    m_openDescription = Localized("opens_tomorrow_at", {time(info.nextTimeOpen)});
  else if (minutes < 7 * 24 * 60)
    m_openDescription =
        Localized("opens_dayoftheweek_at", {locale.dayName(opens.date().dayOfWeek()), time(info.nextTimeOpen)});
}

void PlacePage::SetNorth(double north)
{
  m_north = north;
  if (m_open)
    UpdateDistance();
}

void PlacePage::UpdateDistance()
{
  QString distance;
  double azimuth = -1.0;
  QString bearing;
  if (m_framework.HasPlacePageInfo())
  {
    if (auto const position = m_framework.GetCurrentPosition())
    {
      auto const & info = m_framework.GetCurrentPlacePageInfo();
      if (!info.IsMyPosition())
      {
        auto const ll = mercator::ToLatLon(*position);
        platform::Distance d;
        double azimut;
        m_framework.GetDistanceAndAzimut(info.GetMercator(), ll.m_lat, ll.m_lon, m_north, d, azimut);
        distance = QString::fromStdString(d.ToString());
        azimuth = math::RadToDeg(azimut);
        bearing = QString::number(std::lround(math::RadToDeg(ang::AngleIn2PI(azimut + m_north)))) + "°";
      }
    }
  }
  if (distance != m_distance || azimuth != m_azimuth || bearing != m_bearing)
  {
    m_distance = distance;
    m_azimuth = azimuth;
    m_bearing = bearing;
    emit distanceChanged();
  }
}
}  // namespace sailfish
