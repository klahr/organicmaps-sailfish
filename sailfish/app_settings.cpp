#include "sailfish/app_settings.hpp"

#include "sailfish/app_info.hpp"
#include "sailfish/file_log.hpp"

#include "map/framework.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/settings.hpp"

#include <QFileInfo>
#include <QNetworkConfiguration>
#include <QNetworkConfigurationManager>
#include <QUrl>
#include <QVariantMap>

namespace sailfish
{
namespace
{
// UI only settings, stored next to the core ones.
std::string_view constexpr kMapAppearance = "SailfishMapAppearance";
std::string_view constexpr kZoomButtons = "SailfishZoomButtons";
std::string_view constexpr kKeepScreenOn = "SailfishKeepScreenOn";
std::string_view constexpr kLogging = "SailfishFileLogging";
// Same setting as on Android.
std::string_view constexpr kAutoNightInNavigation = "AutoDarkNavigation";
std::string_view constexpr kSearchHistory = "SailfishSearchHistory";
std::string_view constexpr kAutoDownload = "SailfishAutoDownload";
std::string_view constexpr kMobileData = "SailfishMobileData";

template <typename T>
T Load(std::string_view key, T defaultValue)
{
  T value;
  return settings::Get(key, value) ? value : defaultValue;
}
}  // namespace

MapStyle BaseMapStyle(bool dark, bool outdoors)
{
  if (outdoors)
    return dark ? MapStyleOutdoorsDark : MapStyleOutdoorsLight;
  return dark ? MapStyleDefaultDark : MapStyleDefaultLight;
}

AppSettings::AppSettings(Framework & framework, QObject * parent) : QObject(parent), m_framework(framework) {}

int AppSettings::mapAppearance() const
{
  return Load<int>(kMapAppearance, AppearanceAuto);
}

void AppSettings::setMapAppearance(int appearance)
{
  settings::Set(kMapAppearance, appearance);
  emit changed();
}

void AppSettings::applyMapAppearance(bool dark)
{
  MapStyle const current = m_framework.GetMapStyle();
  MapStyle const style = dark ? GetDarkMapStyleVariant(current) : GetLightMapStyleVariant(current);
  if (style != current)
    m_framework.SetMapStyle(style);
}

int AppSettings::units() const
{
  return static_cast<int>(measurement_utils::GetMeasurementUnits());
}

void AppSettings::setUnits(int units)
{
  // Same as UnitLocale.setCurrentUnits() on Android.
  settings::Set(settings::kMeasurementUnits, static_cast<measurement_utils::Units>(units));
  m_framework.SetupMeasurementSystem();
  emit changed();
}

bool AppSettings::zoomButtons() const
{
  return Load(kZoomButtons, true);
}

void AppSettings::setZoomButtons(bool enabled)
{
  settings::Set(kZoomButtons, enabled);
  emit changed();
}

bool AppSettings::showDownloadedRegions() const
{
  return m_framework.IsShowDownloadedRegions();
}

void AppSettings::setShowDownloadedRegions(bool enabled)
{
  m_framework.SetShowDownloadedRegions(enabled);
  emit changed();
}

bool AppSettings::largeFonts() const
{
  return m_framework.LoadLargeFontsSize();
}

void AppSettings::setLargeFonts(bool enabled)
{
  m_framework.SetLargeFontsSize(enabled);
  emit changed();
}

bool AppSettings::transliteration() const
{
  return Framework::LoadTransliteration();
}

void AppSettings::setTransliteration(bool enabled)
{
  Framework::SaveTransliteration(enabled);
  m_framework.AllowTransliteration(enabled);
  emit changed();
}

bool AppSettings::keepScreenOn() const
{
  return Load(kKeepScreenOn, false);
}

void AppSettings::setKeepScreenOn(bool enabled)
{
  settings::Set(kKeepScreenOn, enabled);
  emit changed();
}

bool AppSettings::IsSearchHistoryEnabled()
{
  return Load(kSearchHistory, true);
}

bool AppSettings::searchHistory() const
{
  return IsSearchHistoryEnabled();
}

void AppSettings::setSearchHistory(bool enabled)
{
  settings::Set(kSearchHistory, enabled);
  emit changed();
}

QString AppSettings::mapLanguage() const
{
  return QString::fromStdString(Framework::GetMapLanguageCode());
}

void AppSettings::setMapLanguage(QString const & code)
{
  m_framework.SetMapLanguageCode(code.toStdString());
  emit changed();
}

QString AppSettings::mapLanguageName() const
{
  auto const code = Framework::GetMapLanguageCode();
  auto const name = StringUtf8Multilang::GetLangNameByCode(StringUtf8Multilang::GetLangIndex(code));
  return name.empty() ? QString::fromStdString(code) : ToQString(name);
}

QVariantList AppSettings::mapLanguages() const
{
  // The same list as the Android map language picker.
  QVariantList languages;
  for (auto const & lang : StringUtf8Multilang::GetSupportedLanguages(false /* includeServiceLangs */))
  {
    QVariantMap item;
    item["code"] = ToQString(lang.m_code);
    item["name"] = ToQString(lang.m_name);
    languages.append(item);
  }
  return languages;
}

bool AppSettings::buildings3d() const
{
  bool allow3d, buildings;
  Framework::Load3dMode(allow3d, buildings);
  return buildings;
}

void AppSettings::setBuildings3d(bool enabled)
{
  bool allow3d, buildings;
  Framework::Load3dMode(allow3d, buildings);
  Framework::Save3dMode(allow3d, enabled);
  m_framework.Allow3dMode(allow3d, enabled);
  emit changed();
}

bool AppSettings::perspectiveView() const
{
  bool allow3d, buildings;
  Framework::Load3dMode(allow3d, buildings);
  return allow3d;
}

void AppSettings::setPerspectiveView(bool enabled)
{
  bool allow3d, buildings;
  Framework::Load3dMode(allow3d, buildings);
  Framework::Save3dMode(enabled, buildings);
  m_framework.Allow3dMode(enabled, buildings);
  emit changed();
}

bool AppSettings::logging() const
{
  return file_log::IsEnabled();
}

void AppSettings::setLogging(bool enabled)
{
  settings::Set(kLogging, enabled);
  file_log::Enable(enabled);
  emit changed();
}

QString AppSettings::logUrl() const
{
  return QUrl::fromLocalFile(file_log::Path()).toString();
}

qint64 AppSettings::logSize() const
{
  return QFileInfo(file_log::Path()).size();
}

void AppSettings::InitLogging()
{
  file_log::Enable(Load(kLogging, false));
}

bool AppSettings::autoNightInNavigation() const
{
  return Load(kAutoNightInNavigation, false);
}

void AppSettings::setAutoNightInNavigation(bool enabled)
{
  settings::Set(kAutoNightInNavigation, enabled);
  emit changed();
}

bool AppSettings::autoZoom() const
{
  return m_framework.LoadAutoZoom();
}

void AppSettings::setAutoZoom(bool enabled)
{
  m_framework.AllowAutoZoom(enabled);
  m_framework.SaveAutoZoom(enabled);
  emit changed();
}

bool AppSettings::IsAutoDownloadEnabled()
{
  return Load(kAutoDownload, true);
}

void AppSettings::setAutoDownload(bool enabled)
{
  settings::Set(kAutoDownload, enabled);
  emit changed();
}

int AppSettings::mobileData() const
{
  return Load(kMobileData, static_cast<int>(MobileDataAsk));
}

void AppSettings::setMobileData(int mobileData)
{
  settings::Set(kMobileData, mobileData);
  emit changed();
}

int AppSettings::downloadPermission() const
{
  if (!IsOnMobileData())
    return DownloadAllowed;
  switch (mobileData())
  {
  case MobileDataAlways: return DownloadAllowed;
  case MobileDataNever: return DownloadDenied;
  default: return DownloadAsk;
  }
}

// static
bool AppSettings::IsOnMobileData()
{
  switch (QNetworkConfigurationManager().defaultConfiguration().bearerTypeFamily())
  {
  case QNetworkConfiguration::Bearer2G:
  case QNetworkConfiguration::Bearer3G:
  case QNetworkConfiguration::Bearer4G: return true;
  default: return false;
  }
}

int AppSettings::powerScheme() const
{
  return static_cast<int>(m_framework.GetPowerManager().GetScheme());
}

void AppSettings::setPowerScheme(int scheme)
{
  m_framework.GetPowerManager().SetScheme(static_cast<power_management::Scheme>(scheme));
  emit changed();
}

int AppSettings::speedCamerasMode() const
{
  return static_cast<int>(m_framework.GetRoutingManager().GetSpeedCamManager().GetMode());
}

void AppSettings::setSpeedCamerasMode(int mode)
{
  m_framework.GetRoutingManager().GetSpeedCamManager().SetMode(static_cast<routing::SpeedCameraManagerMode>(mode));
  emit changed();
}

int AppSettings::bookmarksTextPlacement() const
{
  return static_cast<int>(Framework::GetBookmarksTextPlacement());
}

void AppSettings::setBookmarksTextPlacement(int placement)
{
  m_framework.SetBookmarksTextPlacement(static_cast<settings::Placement>(placement));
  emit changed();
}

bool AppSettings::bgTilesEnabled() const
{
  return Framework::IsBackgroundTilesEnabled();
}

QString AppSettings::bgTilesUrl() const
{
  return QString::fromStdString(Framework::GetBackgroundTilesURL());
}

int AppSettings::bgTilesCacheSize() const
{
  return static_cast<int>(Framework::GetBackgroundTilesCacheSize());
}

int AppSettings::bgTilesOpacity() const
{
  return static_cast<int>(Framework::GetBackgroundTilesAreaOpacity());
}

void AppSettings::setBackgroundTiles(bool enabled, QString const & url, int cacheSizeMb, int opacityPct)
{
  m_framework.SetBackgroundTiles(enabled, url.trimmed().toStdString(), static_cast<uint32_t>(cacheSizeMb),
                                 static_cast<uint32_t>(opacityPct));
  emit changed();
}

bool AppSettings::isWellFormedTilesUrl(QString const & url) const
{
  return Framework::IsWellFormedBackgroundTilesURL(url.trimmed().toStdString());
}

QString AppSettings::donateUrl() const
{
  return QString::fromStdString(Load<std::string>(settings::kDonateUrl, {}));
}
}  // namespace sailfish
