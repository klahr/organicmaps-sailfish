#pragma once

#include "storage/storage.hpp"

#include "platform/downloader_defines.hpp"
#include "platform/settings.hpp"

#include "drape/color.hpp"

#include <QString>
#include <QStringList>
#include <QVariantList>

#include <optional>
#include <string_view>

class BookmarkManager;
class ElevationInfo;

namespace sailfish
{
// A setting, or defaultValue when it isn't set.
template <typename T>
T LoadSetting(std::string_view key, T defaultValue)
{
  T value;
  return settings::Get(key, value) ? value : defaultValue;
}

// The storage attributes of a map, and the share of its download done, 0 when unknown.
storage::NodeAttrs MapAttrs(storage::Storage const & storage, storage::CountryId const & countryId);
double ProgressFraction(downloader::Progress const & progress);

// "#rrggbb" for QML.
QString ColorName(dp::Color const & color);
// The preset colors of bookmarks and tracks, in the order of the pickers of the other platforms; a preset by its
// index there, none for another index.
QStringList PresetColors();
std::optional<dp::Color> PresetColor(int index);
// The index of a preset color, -1 for another color.
int PresetIndex(dp::Color const & color);
// Distance and altitude pairs in meters in one flat list, thinned to what a phone wide chart shows. The last
// point is kept, so that the profile ends at length, which is set when given.
QVariantList ElevationProfile(ElevationInfo const & elevation, double * length = nullptr);
// Bookmark lists as {id, name}, for the list choosers.
QVariantList BookmarkLists(BookmarkManager const & manager);
// A new list, which new bookmarks then go to, like on Android.
uint64_t CreateBookmarkList(BookmarkManager & manager, QString const & name);
// Like "42 km/h", in the measurement units.
QString FormatSpeed(double metersPerSecond);
// Like "▲120 m", as on the Android "my position" page.
QString FormatAltitude(double meters);
// Like "59.32932, 18.06858".
QString FormatLatLon(double lat, double lon);
}  // namespace sailfish
