#include "sailfish/helpers.hpp"

#include "map/bookmark_manager.hpp"
#include "map/elevation_info.hpp"

#include "platform/distance.hpp"
#include "platform/localization.hpp"
#include "platform/measurement_utils.hpp"

#include "kml/types.hpp"

#include <QVariantMap>

#include <algorithm>
#include <iterator>

namespace sailfish
{
storage::NodeAttrs MapAttrs(storage::Storage const & storage, storage::CountryId const & countryId)
{
  storage::NodeAttrs attrs;
  storage.GetNodeAttrs(countryId, attrs);
  return attrs;
}

double ProgressFraction(downloader::Progress const & progress)
{
  if (progress.IsUnknown() || progress.m_bytesTotal <= 0)
    return 0.0;
  return static_cast<double>(progress.m_bytesDownloaded) / progress.m_bytesTotal;
}

QString ColorName(dp::Color const & color)
{
  return QStringLiteral("#%1").arg(color.GetRGBA() >> 8, 6, 16, QLatin1Char('0'));
}

QStringList PresetColors()
{
  QStringList colors;
  for (auto const preset : kml::kOrderedPredefinedColors)
    colors.append(ColorName(kml::ColorFromPredefinedColor(preset)));
  return colors;
}

std::optional<dp::Color> PresetColor(int index)
{
  auto const & presets = kml::kOrderedPredefinedColors;
  if (index < 0 || index >= static_cast<int>(presets.size()))
    return {};
  return kml::ColorFromPredefinedColor(presets[static_cast<size_t>(index)]);
}

int PresetIndex(dp::Color const & color)
{
  auto const & presets = kml::kOrderedPredefinedColors;
  auto const it = std::find_if(presets.begin(), presets.end(), [&color](kml::PredefinedColor preset)
  { return kml::ColorFromPredefinedColor(preset).GetRGBA() == color.GetRGBA(); });
  return it != presets.end() ? static_cast<int>(std::distance(presets.begin(), it)) : -1;
}

QVariantList ElevationProfile(ElevationInfo const & elevation, double * length)
{
  size_t constexpr kMaxPoints = 600;
  QVariantList profile;
  size_t const count = elevation.GetSize();
  size_t const step = count / kMaxPoints + 1;
  size_t i = 0;
  elevation.ForEachPoint([&](double distance, geometry::Altitude altitude)
  {
    if (i % step == 0 || i + 1 == count)
      profile << distance << altitude;
    ++i;
    if (length)
      *length = distance;
  });
  return profile;
}

QVariantList BookmarkLists(BookmarkManager const & manager)
{
  QVariantList lists;
  for (auto const id : manager.GetSortedBmGroupIdList())
  {
    lists.append(QVariantMap{{"id", QVariant::fromValue<quint64>(id)},
                             {"name", QString::fromStdString(manager.GetCategoryName(id))}});
  }
  return lists;
}

uint64_t CreateBookmarkList(BookmarkManager & manager, QString const & name)
{
  auto const id = manager.CreateBookmarkCategory(name.trimmed().toStdString());
  manager.SetLastEditedBmCategory(id);
  return id;
}

QString FormatSpeed(double metersPerSecond)
{
  auto const units = measurement_utils::GetMeasurementUnits();
  return QString::fromStdString(measurement_utils::FormatSpeedNumeric(metersPerSecond, units) + " " +
                                platform::GetLocalizedSpeedUnits(units));
}

QString FormatAltitude(double meters)
{
  return QStringLiteral("▲") + QString::fromStdString(platform::Distance::FormatAltitude(meters));
}

QString FormatLatLon(double lat, double lon)
{
  return QStringLiteral("%1, %2").arg(lat, 0, 'f', 5).arg(lon, 0, 'f', 5);
}
}  // namespace sailfish
