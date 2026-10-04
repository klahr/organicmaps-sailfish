#pragma once

#include <QObject>
#include <QStringList>

#include <cstdint>
#include <vector>

class Framework;

namespace sailfish
{
// Edits the name, notes, color and list of a bookmark or a track, like EditBookmarkFragment on Android.
class BookmarkEditor : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool isTrack READ isTrack NOTIFY loaded)
  Q_PROPERTY(QString name READ name NOTIFY loaded)
  Q_PROPERTY(QString description READ description NOTIFY loaded)
  // Index in appInfo.bookmarkColors, or -1 for a custom color, which is kept unless another one is chosen.
  Q_PROPERTY(int colorIndex READ colorIndex NOTIFY loaded)
  Q_PROPERTY(int categoryIndex READ categoryIndex NOTIFY loaded)
  Q_PROPERTY(QStringList categories READ categories NOTIFY loaded)
  // A track is shown on the map, like the toggle of the Android track editor.
  Q_PROPERTY(bool trackVisible READ trackVisible WRITE setTrackVisible NOTIFY loaded)

public:
  explicit BookmarkEditor(QObject * parent = nullptr);

  bool trackVisible() const;
  void setTrackVisible(bool visible);
  Q_INVOKABLE void loadBookmark(quint64 id);
  Q_INVOKABLE void loadTrack(quint64 id);
  Q_INVOKABLE void save(QString const & name, QString const & description, int colorIndex, int categoryIndex);
  Q_INVOKABLE void remove();
  // Adds a list and returns its index in categories, like "Add a New List" in the list chooser.
  Q_INVOKABLE int createCategory(QString const & name);

  bool isTrack() const { return m_isTrack; }
  QString name() const { return m_name; }
  QString description() const { return m_description; }
  int colorIndex() const { return m_colorIndex; }
  int categoryIndex() const;
  QStringList categories() const;

signals:
  void loaded();

private:
  void LoadCategories(uint64_t groupId);
  // Index of a list in categories, -1 for none.
  int CategoryIndex(uint64_t groupId) const;
  // Whether the place page shows the edited bookmark or track.
  bool IsSelected() const;

  Framework & m_framework;
  bool m_isTrack = false;
  uint64_t m_id = 0;
  uint64_t m_groupId = 0;
  QString m_name;
  QString m_description;
  int m_colorIndex = -1;
  std::vector<uint64_t> m_categoryIds;
};
}  // namespace sailfish
