#include "sailfish/bookmarks_io.hpp"

#include "sailfish/app_info.hpp"
#include "sailfish/bookmarks_model.hpp"

#include "map/bookmark_helpers.hpp"
#include "map/bookmark_manager.hpp"
#include "map/framework.hpp"

#include "platform/settings.hpp"

#include "base/logging.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QStandardPaths>
#include <QUrl>

#include <array>
#include <string_view>

namespace sailfish
{
namespace
{
std::string_view constexpr kBackupPeriodSetting = "SailfishBackupPeriod";
std::string_view constexpr kLastBackupSetting = "SailfishLastBackup";
std::string_view constexpr kBackupFolderSetting = "SailfishBackupFolder";
int constexpr kDefaultBackupPeriodDays = 7;
int constexpr kBackupsToKeep = 10;
auto constexpr kImportExtensions =
    std::to_array({kKmzExtension, kKmlExtension, kKmbExtension, kGpxExtension, kGeoJsonExtension, kJsonExtension});

QString LocalPath(QString const & file)
{
  QUrl const url(file);
  return url.isLocalFile() ? url.toLocalFile() : file;
}

bool IsImportable(QString const & path)
{
  for (auto const ext : kImportExtensions)
    if (path.endsWith(ToQString(ext), Qt::CaseInsensitive))
      return true;
  return false;
}
}  // namespace

BookmarksIO::BookmarksIO(Framework & framework, QObject * parent) : QObject(parent), m_framework(framework)
{
  auto & notifier = BookmarksNotifier::Instance();
  connect(&notifier, &BookmarksNotifier::fileLoaded, this, [this](QString const & path, bool success)
  {
    QString const name = QFileInfo(path).fileName();
    emit importFinished(success, success ? Localized("load_kmz_successful", {name}) : Localized("load_kmz_failed"));
  });
  connect(&notifier, &BookmarksNotifier::loaded, this, &BookmarksIO::BackUpIfDue);
}

QStringList BookmarksIO::importFilters() const
{
  QStringList filters;
  for (auto const ext : kImportExtensions)
    filters.append('*' + ToQString(ext));
  return filters;
}

QString BookmarksIO::backupFolder() const
{
  std::string folder;
  if (settings::Get(kBackupFolderSetting, folder) && !folder.empty())
    return QString::fromStdString(folder);
  return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QStringLiteral("/Organic Maps");
}

void BookmarksIO::setBackupFolder(QString const & folder)
{
  settings::Set(kBackupFolderSetting, folder.toStdString());
  emit backupChanged();
}

int BookmarksIO::backupPeriod() const
{
  int days = kDefaultBackupPeriodDays;
  settings::TryGet(kBackupPeriodSetting, days);
  return days;
}

void BookmarksIO::setBackupPeriod(int days)
{
  settings::Set(kBackupPeriodSetting, days);
  emit backupChanged();
  BackUpIfDue();
}

QDateTime BookmarksIO::lastBackup() const
{
  int64_t seconds = 0;
  return settings::Get(kLastBackupSetting, seconds) ? QDateTime::fromMSecsSinceEpoch(seconds * 1000) : QDateTime();
}

void BookmarksIO::importFile(QString const & file)
{
  m_framework.GetBookmarkManager().LoadBookmark(LocalPath(file).toStdString(), false /* isTemporaryFile */);
}

bool BookmarksIO::OpenFile(QString const & file)
{
  QString const path = LocalPath(file);
  if (!QFileInfo(path).isFile() || !IsImportable(path))
    return false;
  importFile(path);
  return true;
}

namespace
{
// Handles a SharingResult on the GUI thread, unless the receiver is gone.
BookmarkManager::SharingHandler MakeSharingHandler(QPointer<BookmarksIO> io)
{
  return [io](BookmarkManager::SharingResult const & result)
  {
    if (!io)
      return;
    using Code = BookmarkManager::SharingResult::Code;
    switch (result.m_code)
    {
    case Code::Success:
      emit io->exportReady(QUrl::fromLocalFile(QString::fromStdString(result.m_sharingPath)).toString(),
                           QString::fromStdString(result.m_mimeType));
      break;
    case Code::EmptyCategory: emit io->exportFailed(Localized("bookmarks_error_title_share_empty")); break;
    default: emit io->exportFailed(Localized("dialog_routing_system_error")); break;
    }
  };
}

FileType ToFileType(int fileType)
{
  return fileType == BookmarksIO::Gpx ? FileType::Gpx : FileType::Kml;
}
}  // namespace

void BookmarksIO::exportCategory(quint64 categoryId, int fileType)
{
  m_framework.GetBookmarkManager().PrepareFileForSharing({categoryId}, MakeSharingHandler(this), ToFileType(fileType));
}

void BookmarksIO::exportTrack(quint64 trackId, int fileType)
{
  m_framework.GetBookmarkManager().PrepareTrackFileForSharing(trackId, MakeSharingHandler(this), ToFileType(fileType));
}

void BookmarksIO::exportAll()
{
  m_framework.GetBookmarkManager().PrepareAllFilesForSharing(MakeSharingHandler(this));
}

void BookmarksIO::backUpNow()
{
  auto & manager = m_framework.GetBookmarkManager();
  if (m_backingUp || manager.AreAllCategoriesEmpty())
    return;
  m_backingUp = true;
  emit backupChanged();
  QPointer<BookmarksIO> self(this);
  manager.PrepareAllFilesForSharing([self](BookmarkManager::SharingResult const & result)
  {
    if (!self)
      return;
    bool const success = result.m_code == BookmarkManager::SharingResult::Code::Success;
    self->OnBackupFile(success ? QString::fromStdString(result.m_sharingPath) : QString());
  });
}

void BookmarksIO::OnBackupFile(QString const & path)
{
  m_backingUp = false;
  QDir const dir(backupFolder());
  QString const target =
      dir.filePath(QStringLiteral("bookmarks-%1.kmz").arg(QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss")));
  bool const success = !path.isEmpty() && QDir().mkpath(dir.path()) && QFile::copy(path, target);
  if (!path.isEmpty())
    QFile::remove(path);
  if (success)
  {
    settings::Set(kLastBackupSetting, static_cast<int64_t>(QDateTime::currentMSecsSinceEpoch() / 1000));
    // The names sort by time, keep the newest.
    auto const backups = dir.entryList({QStringLiteral("bookmarks-*.kmz")}, QDir::Files, QDir::Name | QDir::Reversed);
    for (int i = kBackupsToKeep; i < backups.size(); ++i)
      QFile::remove(dir.filePath(backups[i]));
  }
  else
  {
    LOG(LWARNING, ("Bookmarks backup to", target.toStdString(), "failed"));
  }
  emit backupChanged();
  emit backupFinished(success);
}

void BookmarksIO::BackUpIfDue()
{
  int const days = backupPeriod();
  if (days <= 0)
    return;
  QDateTime const last = lastBackup();
  if (!last.isValid() || last.addDays(days) <= QDateTime::currentDateTime())
    backUpNow();
}
}  // namespace sailfish
