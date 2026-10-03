#pragma once

#include <QAbstractListModel>
#include <QVariantList>

#include <cstdint>
#include <vector>

class Framework;

namespace sailfish
{
// Forwards BookmarkManager changes (edits, finished loading) to every bookmarks model.
class BookmarksNotifier : public QObject
{
  Q_OBJECT

public:
  static BookmarksNotifier & Instance();
  // Installs the BookmarkManager callbacks and starts loading the bookmarks.
  static void LoadBookmarks(Framework & framework);

signals:
  void changed();
  // Loading at startup finished.
  void loaded();
  // A file given to BookmarkManager::LoadBookmark() was imported or failed.
  void fileLoaded(QString const & path, bool success);
};

// Bookmark lists, as on the Android "Bookmarks and Tracks" screen.
class BookmarkCategoriesModel : public QAbstractListModel
{
  Q_OBJECT
  // For "Show all" / "Hide all", like on Android.
  Q_PROPERTY(bool allVisible READ allVisible NOTIFY allVisibleChanged)

public:
  enum Roles
  {
    IdRole = Qt::UserRole + 1,
    NameRole,
    BookmarksCountRole,
    TracksCountRole,
    VisibleRole
  };

  explicit BookmarkCategoriesModel(QObject * parent = nullptr);

  int rowCount(QModelIndex const & parent = {}) const override;
  QVariant data(QModelIndex const & index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  bool allVisible() const;

  Q_INVOKABLE void setVisible(int row, bool visible);
  Q_INVOKABLE void setAllVisible(bool visible);
  Q_INVOKABLE void createCategory(QString const & name);
  Q_INVOKABLE void deleteCategory(int row);
  Q_INVOKABLE void showOnMap(int row);

signals:
  void allVisibleChanged();

private:
  void Reset();

  Framework & m_framework;
  std::vector<uint64_t> m_ids;
};

// Tracks and bookmarks of one list, sorted and filtered like the Android bookmark list.
class BookmarksModel : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY(quint64 categoryId READ categoryId WRITE setCategoryId NOTIFY categoryIdChanged)
  Q_PROPERTY(QString name READ name NOTIFY categoryInfoChanged)
  Q_PROPERTY(QString description READ description NOTIFY categoryInfoChanged)
  // A SortingType, or -1 for the default order: tracks, then bookmarks.
  Q_PROPERTY(int sortingType READ sortingType WRITE setSortingType NOTIFY sortingTypeChanged)
  // SortingType values offered for the list.
  Q_PROPERTY(QVariantList sortingTypes READ sortingTypes NOTIFY categoryInfoChanged)
  // Shows only the items with this text in their names.
  Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)

public:
  enum Roles
  {
    IdRole = Qt::UserRole + 1,
    IsTrackRole,
    NameRole,
    // The feature type of a bookmark, the length of a track.
    TypeRole,
    // The section of a sorted list, like "Tracks" or "A week ago".
    BlockRole
  };

  // Mirrors BookmarkManager::SortingType.
  enum SortingType
  {
    ByType,
    ByDistance,
    ByTime,
    ByName
  };
  Q_ENUM(SortingType)

  explicit BookmarksModel(QObject * parent = nullptr);

  int rowCount(QModelIndex const & parent = {}) const override;
  QVariant data(QModelIndex const & index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  quint64 categoryId() const { return m_categoryId; }
  void setCategoryId(quint64 id);
  QString name() const;
  QString description() const;
  int sortingType() const { return m_sortingType; }
  void setSortingType(int type);
  QVariantList sortingTypes() const;
  QString filter() const { return m_filter; }
  void setFilter(QString const & filter);

  Q_INVOKABLE void showOnMap(int row);
  Q_INVOKABLE void remove(int row);
  // Renames the list and sets its description, like the Android list settings.
  Q_INVOKABLE void setCategoryInfo(QString const & name, QString const & description);

signals:
  void categoryIdChanged();
  void categoryInfoChanged();
  void sortingTypeChanged();
  void filterChanged();

private:
  struct Item
  {
    uint64_t m_id;
    bool m_isTrack;
    QString m_block;
  };

  void Reset();
  void SetItems(std::vector<Item> && items);
  bool Matches(Item const & item) const;
  QString ItemName(Item const & item) const;

  Framework & m_framework;
  quint64 m_categoryId = 0;
  int m_sortingType = -1;
  QString m_filter;
  // Sorting runs in the background; results of an older request are dropped.
  int m_sortRequest = 0;
  std::vector<Item> m_items;
};
}  // namespace sailfish
