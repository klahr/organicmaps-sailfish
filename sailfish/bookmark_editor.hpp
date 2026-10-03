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
  // Index in colors, or -1 for a custom color, which is kept unless another one is chosen.
  Q_PROPERTY(int colorIndex READ colorIndex NOTIFY loaded)
  // The preset colors as "#rrggbb", in the order of the other platforms.
  Q_PROPERTY(QStringList colors READ colors CONSTANT)
  Q_PROPERTY(int categoryIndex READ categoryIndex NOTIFY loaded)
  Q_PROPERTY(QStringList categories READ categories NOTIFY loaded)

public:
  explicit BookmarkEditor(QObject * parent = nullptr);

  Q_INVOKABLE void loadBookmark(quint64 id);
  Q_INVOKABLE void loadTrack(quint64 id);
  Q_INVOKABLE void save(QString const & name, QString const & description, int colorIndex, int categoryIndex);
  Q_INVOKABLE void remove();

  bool isTrack() const { return m_isTrack; }
  QString name() const { return m_name; }
  QString description() const { return m_description; }
  int colorIndex() const { return m_colorIndex; }
  QStringList colors() const;
  int categoryIndex() const;
  QStringList categories() const;

signals:
  void loaded();

private:
  void LoadCategories(uint64_t groupId);
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
