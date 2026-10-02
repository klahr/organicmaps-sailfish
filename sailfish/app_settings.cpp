#include "sailfish/app_settings.hpp"

#include "map/framework.hpp"

#include "indexer/map_style.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/settings.hpp"

#include <QVariantMap>

namespace sailfish
{
namespace
{
// UI only settings, stored next to the core ones.
std::string_view constexpr kMapAppearance = "SailfishMapAppearance";
std::string_view constexpr kZoomButtons = "SailfishZoomButtons";
std::string_view constexpr kKeepScreenOn = "SailfishKeepScreenOn";
std::string_view constexpr kSearchHistory = "SailfishSearchHistory";
std::string_view constexpr kAutoDownload = "SailfishAutoDownload";

template <typename T>
T Load(std::string_view key, T defaultValue)
{
  T value;
  return settings::Get(key, value) ? value : defaultValue;
}
}  // namespace

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

void AppSettings::applyMapAppearance(bool isAmbienceDark)
{
  int const appearance = mapAppearance();
  bool const dark = appearance == AppearanceAuto ? isAmbienceDark : appearance == AppearanceDark;
  MapStyle const current = m_framework.GetMapStyle();
  MapStyle const style = dark ? GetDarkMapStyleVariant(current) : GetLightMapStyleVariant(current);
  if (style != current)
    m_framework.SetMapStyle(style);
}

int AppSettings::units() const
{
  return static_cast<int>(Load(settings::kMeasurementUnits, measurement_utils::Units::Metric));
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
  for (auto const & lang : StringUtf8Multilang::GetSupportedLanguages(false /* includeServiceLangs */))
    if (lang.m_code == code)
      return QString::fromUtf8(lang.m_name.data(), static_cast<int>(lang.m_name.size()));
  return QString::fromStdString(code);
}

QVariantList AppSettings::mapLanguages() const
{
  // The same list as the Android map language picker.
  QVariantList languages;
  for (auto const & lang : StringUtf8Multilang::GetSupportedLanguages(false /* includeServiceLangs */))
  {
    QVariantMap item;
    item["code"] = QString::fromUtf8(lang.m_code.data(), static_cast<int>(lang.m_code.size()));
    item["name"] = QString::fromUtf8(lang.m_name.data(), static_cast<int>(lang.m_name.size()));
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

int AppSettings::powerScheme() const
{
  return static_cast<int>(m_framework.GetPowerManager().GetScheme());
}

void AppSettings::setPowerScheme(int scheme)
{
  m_framework.GetPowerManager().SetScheme(static_cast<power_management::Scheme>(scheme));
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
