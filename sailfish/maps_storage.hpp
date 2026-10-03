#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

#include <thread>

class Framework;

namespace sailfish
{
// Where maps are kept: the app data folder or a memory card, like the Android "Save maps to" setting. Moving
// takes the whole writable folder (maps, edits, track); the app restarts to use it. Available as mapsStorage.
class MapsStorage : public QObject
{
  Q_OBJECT
  // Locations as {path, name, details, current}.
  Q_PROPERTY(QVariantList locations READ locations NOTIFY changed)
  Q_PROPERTY(QString currentName READ currentName NOTIFY changed)
  Q_PROPERTY(bool moving READ moving NOTIFY changed)

public:
  explicit MapsStorage(Framework & framework, QObject * parent = nullptr);
  ~MapsStorage() override;

  // The folder chosen before, when it is still there; read before the platform starts.
  static QString ConfiguredDir();

  QVariantList locations() const;
  QString currentName() const;
  bool moving() const { return m_moving; }

  // Moves the maps into the folder of a location; moveFinished() follows. Fails while maps download.
  Q_INVOKABLE void moveTo(QString const & path);
  // Refreshes the locations, e.g. after a memory card was inserted.
  Q_INVOKABLE void refresh() { emit changed(); }

signals:
  void changed();
  // On success the app must restart, the old folder is gone.
  void moveFinished(bool success);
  // From the moving thread.
  void moved(QString const & path, bool success);

private:
  void OnMoved(QString const & path, bool success);

  Framework & m_framework;
  bool m_moving = false;
  std::thread m_thread;
};
}  // namespace sailfish
