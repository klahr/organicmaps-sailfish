#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QStringList>

class Framework;

namespace sailfish
{
// Bookmark and track files: import, export for sharing and backups. Available to QML as bookmarksIO.
class BookmarksIO : public QObject
{
  Q_OBJECT
  // Name filters of the files that can be imported, for the file picker.
  Q_PROPERTY(QStringList importFilters READ importFilters CONSTANT)
  // Backups of all lists are KMZ files in this folder; the newest ones are kept.
  Q_PROPERTY(QString backupFolder READ backupFolder WRITE setBackupFolder NOTIFY backupChanged)
  // Days between automatic backups, 0 for none.
  Q_PROPERTY(int backupPeriod READ backupPeriod WRITE setBackupPeriod NOTIFY backupChanged)
  Q_PROPERTY(QDateTime lastBackup READ lastBackup NOTIFY backupChanged)
  Q_PROPERTY(bool backingUp READ backingUp NOTIFY backupChanged)

public:
  // Mirrors the kml::FileType values offered for export, as on Android.
  enum FileType
  {
    Kmz,
    Gpx
  };
  Q_ENUM(FileType)

  explicit BookmarksIO(Framework & framework, QObject * parent = nullptr);

  QStringList importFilters() const;
  QString backupFolder() const;
  void setBackupFolder(QString const & folder);
  int backupPeriod() const;
  void setBackupPeriod(int days);
  QDateTime lastBackup() const;
  bool backingUp() const { return m_backingUp; }

  // Adds the lists of a KML, KMZ, KMB, GPX or GeoJSON file, given as a path or a file:// URL.
  Q_INVOKABLE void importFile(QString const & file);
  // Writes a file to share; exportReady() or exportFailed() follows.
  Q_INVOKABLE void exportCategory(quint64 categoryId, int fileType);
  Q_INVOKABLE void exportTrack(quint64 trackId, int fileType);
  Q_INVOKABLE void exportAll();
  Q_INVOKABLE void backUpNow();

  // Imports files given to the app, e.g. opened in the file manager. Returns false for other URLs.
  bool OpenFile(QString const & file);
  // Backs up when the period has passed; called once the bookmarks are loaded.
  void BackUpIfDue();

signals:
  void exportReady(QString const & fileUrl, QString const & mimeType);
  void exportFailed(QString const & message);
  // Import results with the load_kmz_* wording.
  void importFinished(bool success, QString const & message);
  void backupChanged();
  void backupFinished(bool success);

private:
  void OnBackupFile(QString const & path);

  Framework & m_framework;
  bool m_backingUp = false;
};
}  // namespace sailfish
