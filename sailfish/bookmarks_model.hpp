#pragma once

#include <QAbstractListModel>
#include <QStringList>
#include <QVariantList>

#include <cstdint>
#include <vector>

class Framework;

namespace sailfish
{
// The preset colors of bookmarks and tracks as "#rrggbb", in the order of the pickers of the other platforms.
QStringList PresetColors();

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
  // Deleted lists stay in the trash until deleted there, as on iOS.
  Q_PROPERTY(int recentlyDeletedCount READ recentlyDeletedCount NOTIFY recentlyDeletedChanged)

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
  int recentlyDeletedCount() const;

  Q_INVOKABLE void setVisible(int row, bool visible);
  Q_INVOKABLE void setAllVisible(bool visible);
  Q_INVOKABLE void createCategory(QString const & name);
  Q_INVOKABLE void deleteCategory(int row);
  Q_INVOKABLE void showOnMap(int row);
  // Lists in the trash as {name, path, date}.
  Q_INVOKABLE QVariantList recentlyDeleted() const;
  Q_INVOKABLE void recoverDeleted(QStringList const & paths);
  Q_INVOKABLE void deleteForever(QStringList const & paths);

signals:
  void allVisibleChanged();
  void recentlyDeletedChanged();

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
  Q_PROPERTY(QStringList colors READ colors CONSTANT)
  // Lists to move items to as {id, name}.
  Q_PROPERTY(QVariantList categories READ categories NOTIFY categoryInfoChanged)

public:
  enum Roles
  {
    IdRole = Qt::UserRole + 1,
    IsTrackRole,
    NameRole,
    // The feature type of a bookmark, the length of a track.
    TypeRole,
    // The section of a sorted list, like "Tracks" or "A week ago".
    BlockRole,
    // "#rrggbb", like the Android list icons.
    ColorRole,
    // Of a bookmark from the position, "" without one.
    DistanceRole,
    // Of a track on the map.
    VisibleRole
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
  QStringList colors() const { return PresetColors(); }
  QVariantList categories() const;

  Q_INVOKABLE void showOnMap(int row);
  Q_INVOKABLE void remove(int row);
  // The text Android shares for a bookmark: name, address, coordinates and a link.
  Q_INVOKABLE QString shareText(int row) const;
  // Renames the list and sets its description, like the Android list settings.
  Q_INVOKABLE void setCategoryInfo(QString const & name, QString const & description);
  Q_INVOKABLE void setTrackVisible(int row, bool visible);
  // Several items at once, like the Android selection actions.
  Q_INVOKABLE void removeRows(QVariantList const & rows);
  Q_INVOKABLE void moveRows(QVariantList const & rows, quint64 categoryId);
  Q_INVOKABLE void setRowsColor(QVariantList const & rows, int colorIndex);
  // All bookmarks or all tracks of the list, like the Android list settings.
  Q_INVOKABLE void setAllColor(bool tracks, int colorIndex);
  Q_INVOKABLE void moveRowsToNewList(QVariantList const & rows, QString const & name);
  // The list itself, like the iOS list menu: shown on the map, or deleted to the trash.
  Q_INVOKABLE void showListOnMap();
  Q_INVOKABLE void deleteList();

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
  // The bookmarks and tracks of rows, or of all items.
  void CollectIds(QVariantList const & rows, std::vector<uint64_t> & marks, std::vector<uint64_t> & tracks) const;

  Framework & m_framework;
  quint64 m_categoryId = 0;
  int m_sortingType = -1;
  QString m_filter;
  // Sorting runs in the background; results of an older request are dropped.
  int m_sortRequest = 0;
  std::vector<Item> m_items;
};
}  // namespace sailfish
