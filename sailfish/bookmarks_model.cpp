#include "sailfish/bookmarks_model.hpp"

#include "sailfish/framework_access.hpp"

#include "map/bookmark_helpers.hpp"
#include "map/bookmark_manager.hpp"
#include "map/framework.hpp"

#include "kml/type_utils.hpp"
#include "kml/types.hpp"

#include "geometry/mercator.hpp"

#include "platform/distance.hpp"
#include "platform/platform.hpp"

#include <QDateTime>
#include <QPointer>
#include <QVariantMap>

#include <algorithm>
#include <utility>

namespace sailfish
{
namespace
{
static_assert(BookmarksModel::ByType == static_cast<int>(BookmarkManager::SortingType::ByType));
static_assert(BookmarksModel::ByDistance == static_cast<int>(BookmarkManager::SortingType::ByDistance));
static_assert(BookmarksModel::ByTime == static_cast<int>(BookmarkManager::SortingType::ByTime));
static_assert(BookmarksModel::ByName == static_cast<int>(BookmarkManager::SortingType::ByName));

QString ColorName(dp::Color const & color)
{
  return QStringLiteral("#%1").arg(color.GetRGBA() >> 8, 6, 16, QLatin1Char('0'));
}

dp::Color PresetColor(int index)
{
  return kml::ColorFromPredefinedColor(kml::kOrderedPredefinedColors[static_cast<size_t>(index)]);
}
}  // namespace

QStringList PresetColors()
{
  QStringList colors;
  for (auto const preset : kml::kOrderedPredefinedColors)
    colors.append(ColorName(kml::ColorFromPredefinedColor(preset)));
  return colors;
}

BookmarksNotifier & BookmarksNotifier::Instance()
{
  static BookmarksNotifier notifier;
  return notifier;
}

void BookmarksNotifier::LoadBookmarks(Framework & framework)
{
  auto & manager = framework.GetBookmarkManager();
  BookmarkManager::AsyncLoadingCallbacks callbacks;
  callbacks.m_onFinished = []
  {
    emit Instance().changed();
    emit Instance().loaded();
  };
  callbacks.m_onFileSuccess = [](std::string const & path, bool)
  { emit Instance().fileLoaded(QString::fromStdString(path), true); };
  callbacks.m_onFileError = [](std::string const & path, bool)
  { emit Instance().fileLoaded(QString::fromStdString(path), false); };
  manager.SetAsyncLoadingCallbacks(std::move(callbacks));
  manager.SetBookmarksChangedCallback([] { emit Instance().changed(); });
  framework.LoadBookmarks();
}

BookmarkCategoriesModel::BookmarkCategoriesModel(QObject * parent)
  : QAbstractListModel(parent)
  , m_framework(GetFramework())
{
  connect(&BookmarksNotifier::Instance(), &BookmarksNotifier::changed, this, &BookmarkCategoriesModel::Reset);
  Reset();
}

void BookmarkCategoriesModel::Reset()
{
  beginResetModel();
  auto const ids = m_framework.GetBookmarkManager().GetSortedBmGroupIdList();
  m_ids.assign(ids.begin(), ids.end());
  endResetModel();
  emit allVisibleChanged();
  emit recentlyDeletedChanged();
}

bool BookmarkCategoriesModel::allVisible() const
{
  return m_framework.GetBookmarkManager().AreAllCategoriesVisible();
}

void BookmarkCategoriesModel::setAllVisible(bool visible)
{
  m_framework.GetBookmarkManager().SetAllCategoriesVisibility(visible);
  emit dataChanged(index(0), index(rowCount() - 1), {VisibleRole});
  emit allVisibleChanged();
}

int BookmarkCategoriesModel::rowCount(QModelIndex const & parent) const
{
  return parent.isValid() ? 0 : static_cast<int>(m_ids.size());
}

QVariant BookmarkCategoriesModel::data(QModelIndex const & index, int role) const
{
  if (!index.isValid() || index.row() >= rowCount())
    return {};

  auto const id = m_ids[static_cast<size_t>(index.row())];
  auto const & manager = m_framework.GetBookmarkManager();
  switch (role)
  {
  case IdRole: return QVariant::fromValue<quint64>(id);
  case NameRole: return QString::fromStdString(manager.GetCategoryName(id));
  case BookmarksCountRole: return static_cast<int>(manager.GetUserMarkIds(id).size());
  case TracksCountRole: return static_cast<int>(manager.GetTrackIds(id).size());
  case VisibleRole: return manager.IsVisible(id);
  default: return {};
  }
}

QHash<int, QByteArray> BookmarkCategoriesModel::roleNames() const
{
  return {{IdRole, "categoryId"},
          {NameRole, "name"},
          {BookmarksCountRole, "bookmarksCount"},
          {TracksCountRole, "tracksCount"},
          {VisibleRole, "isVisible"}};
}

void BookmarkCategoriesModel::setVisible(int row, bool visible)
{
  if (row < 0 || row >= rowCount())
    return;
  m_framework.GetBookmarkManager().GetEditSession().SetIsVisible(m_ids[static_cast<size_t>(row)], visible);
  // Visibility doesn't trigger the changed callback.
  emit dataChanged(index(row), index(row), {VisibleRole});
  emit allVisibleChanged();
}

void BookmarkCategoriesModel::createCategory(QString const & name)
{
  auto & manager = m_framework.GetBookmarkManager();
  // New bookmarks go to the newest list, like on Android.
  manager.SetLastEditedBmCategory(manager.CreateBookmarkCategory(name.trimmed().toStdString()));
  Reset();
}

void BookmarkCategoriesModel::deleteCategory(int row)
{
  if (row < 0 || row >= rowCount())
    return;
  // To the trash, like on iOS.
  m_framework.GetBookmarkManager().GetEditSession().DeleteBmCategory(m_ids[static_cast<size_t>(row)],
                                                                     false /* permanently */);
  Reset();
}

int BookmarkCategoriesModel::recentlyDeletedCount() const
{
  return static_cast<int>(m_framework.GetBookmarkManager().GetRecentlyDeletedCategoriesCount());
}

QVariantList BookmarkCategoriesModel::recentlyDeleted() const
{
  QVariantList result;
  auto const collection = m_framework.GetBookmarkManager().GetRecentlyDeletedCategories();
  for (auto const & [path, data] : *collection)
  {
    // The file was created when the list was moved to the trash.
    auto const time = Platform::GetFileCreationTime(path);
    result.append(QVariantMap{{"name", QString::fromStdString(GetPreferredBookmarkStr(data->m_categoryData.m_name))},
                              {"path", QString::fromStdString(path)},
                              {"date", time > 0 ? QDateTime::fromTime_t(static_cast<uint>(time)) : QDateTime()}});
  }
  // Newest first.
  std::sort(result.begin(), result.end(), [](QVariant const & a, QVariant const & b)
  { return a.toMap()["date"].toDateTime() > b.toMap()["date"].toDateTime(); });
  return result;
}

void BookmarkCategoriesModel::recoverDeleted(QStringList const & paths)
{
  std::vector<std::string> files;
  for (auto const & path : paths)
    files.push_back(path.toStdString());
  // The recovered lists load in the background; the changed callback follows.
  m_framework.GetBookmarkManager().RecoverRecentlyDeletedCategoriesAtPaths(files);
  emit recentlyDeletedChanged();
}

void BookmarkCategoriesModel::deleteForever(QStringList const & paths)
{
  std::vector<std::string> files;
  for (auto const & path : paths)
    files.push_back(path.toStdString());
  m_framework.GetBookmarkManager().DeleteRecentlyDeletedCategoriesAtPaths(files);
  emit recentlyDeletedChanged();
}

void BookmarkCategoriesModel::showOnMap(int row)
{
  if (row >= 0 && row < rowCount())
    m_framework.ShowBookmarkCategory(m_ids[static_cast<size_t>(row)]);
}

BookmarksModel::BookmarksModel(QObject * parent) : QAbstractListModel(parent), m_framework(GetFramework())
{
  connect(&BookmarksNotifier::Instance(), &BookmarksNotifier::changed, this, &BookmarksModel::Reset);
}

void BookmarksModel::setCategoryId(quint64 id)
{
  if (id == m_categoryId)
    return;
  m_categoryId = id;
  auto const & manager = m_framework.GetBookmarkManager();
  BookmarkManager::SortingType type;
  m_sortingType = manager.HasBmCategory(id) && manager.GetLastSortingType(id, type) ? static_cast<int>(type) : -1;
  emit categoryIdChanged();
  emit categoryInfoChanged();
  emit sortingTypeChanged();
  Reset();
}

QString BookmarksModel::name() const
{
  auto const & manager = m_framework.GetBookmarkManager();
  return manager.HasBmCategory(m_categoryId) ? QString::fromStdString(manager.GetCategoryName(m_categoryId))
                                             : QString();
}

QString BookmarksModel::description() const
{
  auto const & manager = m_framework.GetBookmarkManager();
  if (!manager.HasBmCategory(m_categoryId))
    return {};
  return QString::fromStdString(kml::GetDefaultStr(manager.GetCategoryData(m_categoryId).m_description));
}

void BookmarksModel::setSortingType(int type)
{
  if (type == m_sortingType)
    return;
  m_sortingType = type;
  if (auto & manager = m_framework.GetBookmarkManager(); manager.HasBmCategory(m_categoryId))
  {
    if (type < 0)
      manager.ResetLastSortingType(m_categoryId);
    else
      manager.SetLastSortingType(m_categoryId, static_cast<BookmarkManager::SortingType>(type));
  }
  emit sortingTypeChanged();
  Reset();
}

QVariantList BookmarksModel::sortingTypes() const
{
  auto const & manager = m_framework.GetBookmarkManager();
  QVariantList types;
  if (!manager.HasBmCategory(m_categoryId))
    return types;
  for (auto const type : manager.GetAvailableSortingTypes(m_categoryId, m_framework.GetCurrentPosition().has_value()))
    types.append(static_cast<int>(type));
  return types;
}

void BookmarksModel::setFilter(QString const & filter)
{
  if (filter == m_filter)
    return;
  m_filter = filter;
  emit filterChanged();
  Reset();
}

void BookmarksModel::setCategoryInfo(QString const & name, QString const & description)
{
  auto & manager = m_framework.GetBookmarkManager();
  if (!manager.HasBmCategory(m_categoryId))
    return;
  auto session = manager.GetEditSession();
  if (!name.trimmed().isEmpty())
    session.SetCategoryName(m_categoryId, name.trimmed().toStdString());
  session.SetCategoryDescription(m_categoryId, description.trimmed().toStdString());
  emit categoryInfoChanged();
}

void BookmarksModel::Reset()
{
  int const request = ++m_sortRequest;
  auto & manager = m_framework.GetBookmarkManager();
  if (!manager.HasBmCategory(m_categoryId))
  {
    SetItems({});
    return;
  }

  if (m_sortingType < 0)
  {
    // The default order of the Android list: tracks, then bookmarks, in sections when there are both.
    auto const & trackIds = manager.GetTrackIds(m_categoryId);
    auto const & markIds = manager.GetUserMarkIds(m_categoryId);
    bool const sections = !trackIds.empty() && !markIds.empty();
    QString const tracksBlock = sections ? QString::fromStdString(BookmarkManager::GetTracksSortedBlockName()) : "";
    QString const marksBlock = sections ? QString::fromStdString(BookmarkManager::GetBookmarksSortedBlockName()) : "";
    std::vector<Item> items;
    for (auto const id : trackIds)
      items.push_back({id, true /* isTrack */, tracksBlock});
    for (auto const id : markIds)
      items.push_back({id, false /* isTrack */, marksBlock});
    SetItems(std::move(items));
    return;
  }

  BookmarkManager::SortParams params;
  params.m_groupId = m_categoryId;
  params.m_sortingType = static_cast<BookmarkManager::SortingType>(m_sortingType);
  if (auto const position = m_framework.GetCurrentPosition())
  {
    params.m_hasMyPosition = true;
    params.m_myPosition = *position;
  }
  // Results come on the GUI thread, possibly after the list page is gone.
  QPointer<BookmarksModel> self(this);
  params.m_onResults =
      [self, request](BookmarkManager::SortedBlocksCollection && blocks, BookmarkManager::SortParams::Status status)
  {
    if (!self || request != self->m_sortRequest)
      return;
    if (status != BookmarkManager::SortParams::Status::Completed)
    {
      // E.g. a sorting that is no longer available, like by distance without a position.
      self->setSortingType(-1);
      return;
    }
    std::vector<Item> items;
    for (auto const & block : blocks)
    {
      QString const name = QString::fromStdString(block.m_blockName);
      for (auto const id : block.m_trackIds)
        items.push_back({id, true /* isTrack */, name});
      for (auto const id : block.m_markIds)
        items.push_back({id, false /* isTrack */, name});
    }
    self->SetItems(std::move(items));
  };
  manager.GetSortedCategory(params);
}

void BookmarksModel::SetItems(std::vector<Item> && items)
{
  beginResetModel();
  m_items.clear();
  for (auto & item : items)
    if (Matches(item))
      m_items.push_back(std::move(item));
  endResetModel();
}

bool BookmarksModel::Matches(Item const & item) const
{
  return m_filter.trimmed().isEmpty() || ItemName(item).contains(m_filter.trimmed(), Qt::CaseInsensitive);
}

QString BookmarksModel::ItemName(Item const & item) const
{
  auto const & manager = m_framework.GetBookmarkManager();
  if (item.m_isTrack)
  {
    auto const * track = manager.GetTrack(item.m_id);
    return track ? QString::fromStdString(track->GetName()) : QString();
  }
  auto const * bookmark = manager.GetBookmark(item.m_id);
  return bookmark ? QString::fromStdString(bookmark->GetPreferredName()) : QString();
}

int BookmarksModel::rowCount(QModelIndex const & parent) const
{
  return parent.isValid() ? 0 : static_cast<int>(m_items.size());
}

QVariant BookmarksModel::data(QModelIndex const & index, int role) const
{
  if (!index.isValid() || index.row() >= rowCount())
    return {};

  auto const & item = m_items[static_cast<size_t>(index.row())];
  switch (role)
  {
  case IdRole: return QVariant::fromValue<quint64>(item.m_id);
  case IsTrackRole: return item.m_isTrack;
  case NameRole: return ItemName(item);
  case BlockRole: return item.m_block;
  default: break;
  }

  auto const & manager = m_framework.GetBookmarkManager();
  if (item.m_isTrack)
  {
    auto const * track = manager.GetTrack(item.m_id);
    if (!track)
      return {};
    switch (role)
    {
    case TypeRole:
      return QString::fromStdString(platform::Distance::CreateFormatted(track->GetLengthMeters()).ToString());
    case ColorRole: return ColorName(track->GetColor(0));
    case VisibleRole: return track->IsVisible();
    default: return {};
    }
  }

  auto const * bookmark = manager.GetBookmark(item.m_id);
  if (!bookmark)
    return {};
  switch (role)
  {
  case TypeRole: return QString::fromStdString(kml::GetLocalizedFeatureType(bookmark->GetData().m_featureTypes));
  case ColorRole: return ColorName(bookmark->GetColorForRendering());
  case DistanceRole:
  {
    auto const position = m_framework.GetCurrentPosition();
    if (!position)
      return QString();
    auto const meters = mercator::DistanceOnEarth(*position, bookmark->GetPivot());
    return QString::fromStdString(platform::Distance::CreateFormatted(meters).ToString());
  }
  default: return {};
  }
}

QHash<int, QByteArray> BookmarksModel::roleNames() const
{
  return {{IdRole, "itemId"},   {IsTrackRole, "isTrack"}, {NameRole, "name"},         {TypeRole, "type"},
          {BlockRole, "block"}, {ColorRole, "color"},     {DistanceRole, "distance"}, {VisibleRole, "isVisible"}};
}

void BookmarksModel::showOnMap(int row)
{
  if (row < 0 || row >= rowCount())
    return;
  auto const & item = m_items[static_cast<size_t>(row)];
  if (item.m_isTrack)
    m_framework.ShowTrack(item.m_id);
  else
    m_framework.ShowBookmark(item.m_id);
}

QString BookmarksModel::shareText(int row) const
{
  if (row < 0 || row >= rowCount() || m_items[static_cast<size_t>(row)].m_isTrack)
    return {};
  return QString::fromStdString(m_framework.GetShareDataForBookmark(m_items[static_cast<size_t>(row)].m_id).m_text);
}

void BookmarksModel::remove(int row)
{
  if (row < 0 || row >= rowCount())
    return;
  auto const & item = m_items[static_cast<size_t>(row)];
  auto session = m_framework.GetBookmarkManager().GetEditSession();
  if (item.m_isTrack)
    session.DeleteTrack(item.m_id);
  else
    session.DeleteBookmark(item.m_id);
}
QVariantList BookmarksModel::categories() const
{
  QVariantList result;
  auto const & manager = m_framework.GetBookmarkManager();
  for (auto const id : manager.GetSortedBmGroupIdList())
    result.append(QVariantMap{{"id", QVariant::fromValue<quint64>(id)},
                              {"name", QString::fromStdString(manager.GetCategoryName(id))}});
  return result;
}

void BookmarksModel::setTrackVisible(int row, bool visible)
{
  if (row < 0 || row >= rowCount() || !m_items[static_cast<size_t>(row)].m_isTrack)
    return;
  m_framework.SetTrackVisibility(m_items[static_cast<size_t>(row)].m_id, visible);
  // Visibility doesn't trigger the changed callback.
  emit dataChanged(index(row), index(row), {VisibleRole});
}

void BookmarksModel::CollectIds(QVariantList const & rows, std::vector<uint64_t> & marks,
                                std::vector<uint64_t> & tracks) const
{
  for (auto const & row : rows)
  {
    auto const r = row.toInt();
    if (r < 0 || r >= rowCount())
      continue;
    auto const & item = m_items[static_cast<size_t>(r)];
    (item.m_isTrack ? tracks : marks).push_back(item.m_id);
  }
}

void BookmarksModel::removeRows(QVariantList const & rows)
{
  kml::MarkIdCollection marks;
  kml::TrackIdCollection tracks;
  CollectIds(rows, marks, tracks);
  // Also closes a place page showing a deleted item.
  m_framework.DeleteBookmarksAndTracks(marks, tracks);
}

void BookmarksModel::moveRows(QVariantList const & rows, quint64 categoryId)
{
  kml::MarkIdCollection marks;
  kml::TrackIdCollection tracks;
  CollectIds(rows, marks, tracks);
  m_framework.GetBookmarkManager().GetEditSession().MoveBookmarksAndTracks(marks, tracks, categoryId);
}

void BookmarksModel::setRowsColor(QVariantList const & rows, int colorIndex)
{
  if (colorIndex < 0 || colorIndex >= static_cast<int>(kml::kOrderedPredefinedColors.size()))
    return;
  kml::MarkIdCollection marks;
  kml::TrackIdCollection tracks;
  CollectIds(rows, marks, tracks);
  m_framework.GetBookmarkManager().GetEditSession().SetBookmarksAndTracksColor(marks, tracks, PresetColor(colorIndex));
}

void BookmarksModel::setAllColor(bool tracks, int colorIndex)
{
  if (colorIndex < 0 || colorIndex >= static_cast<int>(kml::kOrderedPredefinedColors.size()))
    return;
  auto const & manager = m_framework.GetBookmarkManager();
  kml::MarkIdCollection marks;
  kml::TrackIdCollection trackIds;
  if (tracks)
  {
    auto const & ids = manager.GetTrackIds(m_categoryId);
    trackIds.assign(ids.begin(), ids.end());
  }
  else
  {
    auto const & ids = manager.GetUserMarkIds(m_categoryId);
    marks.assign(ids.begin(), ids.end());
  }
  m_framework.GetBookmarkManager().GetEditSession().SetBookmarksAndTracksColor(marks, trackIds,
                                                                               PresetColor(colorIndex));
}
}  // namespace sailfish
