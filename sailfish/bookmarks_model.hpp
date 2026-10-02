#pragma once

#include <QAbstractListModel>

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
};

// Bookmark lists, as on the Android "Bookmarks and Tracks" screen.
class BookmarkCategoriesModel : public QAbstractListModel
{
  Q_OBJECT

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

  Q_INVOKABLE void setVisible(int row, bool visible);
  Q_INVOKABLE void createCategory(QString const & name);
  Q_INVOKABLE void deleteCategory(int row);
  Q_INVOKABLE void showOnMap(int row);

private:
  void Reset();

  Framework & m_framework;
  std::vector<uint64_t> m_ids;
};

// Bookmarks of one list.
class BookmarksModel : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY(quint64 categoryId READ categoryId WRITE setCategoryId NOTIFY categoryIdChanged)

public:
  enum Roles
  {
    NameRole = Qt::UserRole + 1,
    TypeRole
  };

  explicit BookmarksModel(QObject * parent = nullptr);

  int rowCount(QModelIndex const & parent = {}) const override;
  QVariant data(QModelIndex const & index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  quint64 categoryId() const { return m_categoryId; }
  void setCategoryId(quint64 id);

  Q_INVOKABLE void showOnMap(int row);
  Q_INVOKABLE void deleteBookmark(int row);

signals:
  void categoryIdChanged();

private:
  void Reset();

  Framework & m_framework;
  quint64 m_categoryId = 0;
  std::vector<uint64_t> m_ids;
};
}  // namespace sailfish
