#include "sailfish/maps_storage.hpp"

#include "sailfish/app_info.hpp"

#include "map/framework.hpp"

#include "storage/storage.hpp"

#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"

#include "base/logging.hpp"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QVariantMap>

#include <vector>

namespace sailfish
{
namespace
{
QString const kStorageKey = QStringLiteral("mapsStorage");

QString DefaultDir()
{
  return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString CurrentDir()
{
  return QDir::cleanPath(QString::fromStdString(GetPlatform().WritableDir()));
}

// Moves every file of the folder, keeping the tree; on failure the copies are removed and the folder is left.
bool MoveTree(QString const & from, QString const & to)
{
  std::vector<std::pair<std::string, std::string>> files;
  for (QDirIterator it(from, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories); it.hasNext();)
  {
    QString const file = it.next();
    files.emplace_back(file.toStdString(), QDir(to).filePath(QDir(from).relativeFilePath(file)).toStdString());
  }

  // Copy all first, so that a full card doesn't leave the maps split.
  std::vector<std::string> copied;
  for (auto const & [source, target] : files)
  {
    if (!QDir().mkpath(QFileInfo(QString::fromStdString(target)).path()) || !base::CopyFileX(source, target))
    {
      LOG(LWARNING, ("Can't copy", source, "to", target));
      for (auto const & file : copied)
        base::DeleteFileX(file);
      return false;
    }
    copied.push_back(target);
  }
  for (auto const & file : files)
    base::DeleteFileX(file.first);
  return true;
}
}  // namespace

MapsStorage::MapsStorage(Framework & framework, QObject * parent) : QObject(parent), m_framework(framework)
{
  connect(this, &MapsStorage::moved, this, &MapsStorage::OnMoved, Qt::QueuedConnection);
}

MapsStorage::~MapsStorage()
{
  if (m_thread.joinable())
    m_thread.join();
}

// static
QString MapsStorage::ConfiguredDir()
{
  QString const dir = QSettings().value(kStorageKey).toString();
  if (dir.isEmpty())
    return {};
  // E.g. the memory card was removed: use the app folder until it is back.
  QFileInfo const info(dir);
  return info.isDir() && info.isWritable() ? dir : QString();
}

QVariantList MapsStorage::locations() const
{
  QString const current = CurrentDir();
  QVariantList result;
  auto const add = [&](QString const & path, QString const & name, QStorageInfo const & volume)
  {
    QString const details =
        Localized("maps_storage_free_size", {FormatSize(volume.bytesAvailable()), FormatSize(volume.bytesTotal())});
    result.append(QVariantMap{
        {"path", path}, {"name", name}, {"details", details}, {"current", QDir::cleanPath(path) == current}});
  };

  add(DefaultDir(), Localized("maps_storage_internal"), QStorageInfo(DefaultDir()));
  // Memory cards and USB sticks, mounted here by Sailfish OS.
  for (auto const & volume : QStorageInfo::mountedVolumes())
  {
    if (!volume.isValid() || !volume.isReady() || volume.isReadOnly() ||
        !volume.rootPath().startsWith(QStringLiteral("/run/media/")))
      continue;
    QString const name = volume.name().isEmpty() ? Localized("maps_storage_removable")
                                                 : Localized("maps_storage_removable") + " (" + volume.name() + ")";
    add(volume.rootPath() + QStringLiteral("/organicmaps"), name, volume);
  }
  return result;
}

QString MapsStorage::downloadedSize() const
{
  auto const & storage = m_framework.GetStorage();
  storage::NodeAttrs attrs;
  storage.GetNodeAttrs(storage.GetRootId(), attrs);
  return FormatSize(static_cast<qint64>(attrs.m_localMwmSize));
}

bool MapsStorage::downloading() const
{
  return m_framework.GetStorage().IsDownloadInProgress();
}

QString MapsStorage::currentName() const
{
  for (auto const & location : locations())
  {
    auto const map = location.toMap();
    if (map["current"].toBool())
      return map["name"].toString();
  }
  return CurrentDir();
}

void MapsStorage::moveTo(QString const & path)
{
  QString const from = CurrentDir();
  QString const to = QDir::cleanPath(path);
  if (m_moving || to == from)
    return;
  if (m_framework.GetStorage().IsDownloadInProgress() || !QDir().mkpath(to))
  {
    emit moveFinished(false);
    return;
  }

  m_moving = true;
  emit changed();
  // The maps are closed while they move, like on Android.
  m_framework.DeregisterAllMaps();
  if (m_thread.joinable())
    m_thread.join();
  m_thread = std::thread([this, from, to] { emit moved(to, MoveTree(from, to)); });
}

void MapsStorage::OnMoved(QString const & path, bool success)
{
  m_moving = false;
  if (success)
    QSettings().setValue(kStorageKey, path == QDir::cleanPath(DefaultDir()) ? QString() : path);
  else
    m_framework.RegisterAllMaps();
  emit changed();
  emit moveFinished(success);
}
}  // namespace sailfish
