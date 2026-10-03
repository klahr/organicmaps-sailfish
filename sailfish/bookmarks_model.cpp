#include "sailfish/bookmarks_model.hpp"

#include "sailfish/framework_access.hpp"

#include "map/bookmark_manager.hpp"
#include "map/framework.hpp"

#include "kml/type_utils.hpp"

#include "platform/distance.hpp"

namespace sailfish
{
BookmarksNotifier & BookmarksNotifier::Instance()
{
  static BookmarksNotifier notifier;
  return notifier;
}

void BookmarksNotifier::LoadBookmarks(Framework & framework)
{
  auto & manager = framework.GetBookmarkManager();
  BookmarkManager::AsyncLoadingCallbacks callbacks;
  callbacks.m_onFinished = [] { emit Instance().changed(); };
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
  m_framework.GetBookmarkManager().GetEditSession().DeleteBmCategory(m_ids[static_cast<size_t>(row)],
                                                                     true /* permanently */);
  Reset();
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
  emit categoryIdChanged();
  Reset();
}

void BookmarksModel::Reset()
{
  beginResetModel();
  m_items.clear();
  auto const & manager = m_framework.GetBookmarkManager();
  if (manager.HasBmCategory(m_categoryId))
  {
    for (auto const id : manager.GetUserMarkIds(m_categoryId))
      m_items.push_back({id, false /* isTrack */});
    for (auto const id : manager.GetTrackIds(m_categoryId))
      m_items.push_back({id, true /* isTrack */});
  }
  endResetModel();
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
    case NameRole: return QString::fromStdString(track->GetName());
    case TypeRole:
      return QString::fromStdString(platform::Distance::CreateFormatted(track->GetLengthMeters()).ToString());
    default: return {};
    }
  }

  auto const * bookmark = manager.GetBookmark(item.m_id);
  if (!bookmark)
    return {};
  switch (role)
  {
  case NameRole: return QString::fromStdString(bookmark->GetPreferredName());
  case TypeRole: return QString::fromStdString(kml::GetLocalizedFeatureType(bookmark->GetData().m_featureTypes));
  default: return {};
  }
}

QHash<int, QByteArray> BookmarksModel::roleNames() const
{
  return {{IdRole, "itemId"}, {IsTrackRole, "isTrack"}, {NameRole, "name"}, {TypeRole, "type"}};
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
}  // namespace sailfish
