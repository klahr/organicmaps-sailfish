#include "sailfish/place_page.hpp"

#include "sailfish/app_info.hpp"

#include "map/bookmark_helpers.hpp"
#include "map/bookmark_manager.hpp"
#include "map/framework.hpp"
#include "map/place_page_info.hpp"

#include "ge0/url_generator.hpp"

#include "opening_hours/opening_hours.hpp"

#include "platform/distance.hpp"
#include "platform/localization.hpp"
#include "platform/settings.hpp"

#include "geometry/mercator.hpp"

#include "base/math.hpp"

#include <QDateTime>
#include <QLocale>
#include <QVariantMap>

namespace sailfish
{
namespace
{
using feature::Metadata;

// Same setting and default as the desktop place page.
std::string_view constexpr kCoordinatesFormatSetting = "CoordinatesFormat";

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
}

PlacePage::~PlacePage()
{
  m_framework.SetPlacePageListeners({}, {}, {}, {});
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
  m_editable = info.CanEditPlace();
  m_shareText = QString::fromStdString(m_framework.GetShareData(info).m_text);
  m_geoUri = QString::fromStdString(
      ge0::GenerateGeoUri(info.GetLatLon().m_lat, info.GetLatLon().m_lon, m_framework.GetDrawScale(), info.GetTitle()));
  UpdateOpeningHours(info.GetOpeningHours());
  m_wikiDescription = QString::fromStdString(info.GetWikiDescription());
  auto const wikipedia = info.GetMetadata(Metadata::FMD_WIKIPEDIA);
  m_wikiUrl = wikipedia.empty() ? QString() : QString::fromStdString(Metadata::ToWikiURL(std::string(wikipedia)));

  m_details.clear();
  auto const add = [this](char const * icon, QString const & text, QString const & url = {})
  {
    if (!text.isEmpty())
      m_details.append(QVariantMap{{"icon", icon}, {"text", text}, {"url", url}});
  };
  // Multiple phone numbers are separated by semicolons.
  for (auto const & phone : ToQString(info.GetMetadata(Metadata::FMD_PHONE_NUMBER)).split(';', QString::SkipEmptyParts))
    add("image://theme/icon-m-phone", phone.trimmed(), "tel:" + phone.trimmed());
  if (auto const website = ToQString(info.GetMetadata(Metadata::FMD_WEBSITE)); !website.isEmpty())
    add("image://theme/icon-m-website", website, website.contains("://") ? website : "https://" + website);
  if (auto const email = ToQString(info.GetMetadata(Metadata::FMD_EMAIL)); !email.isEmpty())
    add("image://theme/icon-m-mail", email, "mailto:" + email);
  if (info.HasWifi())
    add("image://theme/icon-m-wlan", QStringLiteral("Wi-Fi"));
  add("image://theme/icon-m-levels", ToQString(info.GetMetadata(Metadata::FMD_LEVEL)));
  add("image://theme/icon-m-person", ToQString(info.GetMetadata(Metadata::FMD_OPERATOR)));
  if (auto const capacity = info.GetMetadata(Metadata::FMD_CAPACITY); !capacity.empty())
    add("../../icons/placepage/ic_capacity_white.svg", Localized("capacity", {ToQString(capacity)}));
  if (auto const commons = info.GetMetadata(Metadata::FMD_WIKIMEDIA_COMMONS); !commons.empty())
  {
    add("../../icons/placepage/ic_wikimedia_commons_white.svg", Localized("wikimedia_commons"),
        QString::fromStdString(Metadata::ToWikimediaCommonsURL(std::string(commons))));
  }
  // A classificator type like wheelchair-yes, localized like other types.
  if (auto const wheelchair = info.GetMetadata(Metadata::FMD_WHEELCHAIR); !wheelchair.empty())
    add("../../icons/placepage/ic_wheelchair_white.svg",
        QString::fromStdString(platform::GetLocalizedTypeName(std::string(wheelchair))));

  UpdateDistance();
  emit changed();
}

void PlacePage::UpdateOpeningHours(std::string_view openingHours)
{
  m_openingHours = ToQString(openingHours);
  m_openState = OpenUnknown;
  m_openTitle.clear();
  m_openDescription.clear();

  osmoh::OpeningHours const oh(openingHours);
  if (openingHours.empty() || !oh.IsValid())
    return;

  // Local time without the feature time zone, like on Android.
  time_t const now = std::time(nullptr);
  auto const info = oh.GetInfo(now);
  if (info.state == osmoh::RuleState::Unknown)
    return;

  QLocale const locale = QLocale::system();
  auto const time = [&locale](time_t t)
  { return locale.toString(QDateTime::fromTime_t(static_cast<uint>(t)).time(), QLocale::ShortFormat); };

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
      }
    }
  }
  if (distance != m_distance || azimuth != m_azimuth)
  {
    m_distance = distance;
    m_azimuth = azimuth;
    emit distanceChanged();
  }
}
}  // namespace sailfish
