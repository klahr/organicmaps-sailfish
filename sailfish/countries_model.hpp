#pragma once

#include "storage/storage_defines.hpp"

#include <QAbstractListModel>
#include <QString>
#include <QVariantMap>

#include <vector>

namespace downloader
{
struct Progress;
}

namespace storage
{
class Storage;
}

namespace sailfish
{
// A map that isn't downloaded yet, or is on its way, as {countryId, name, size, status, progress}; empty
// otherwise. For the on-map and place page downloaders.
QVariantMap MissingMapInfo(storage::Storage const & storage, storage::CountryId const & countryId);
// Downloads the map, or retries it after an error.
void DownloadMap(storage::Storage & storage, storage::CountryId const & countryId);

// Children of one node of the map download tree (the world root by default), kept in sync with
// storage status and download progress.
class CountriesModel : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY(QString parentId READ parentId WRITE setParentId NOTIFY parentIdChanged)
  Q_PROPERTY(QString title READ title NOTIFY parentIdChanged)
  Q_PROPERTY(bool downloadInProgress READ downloadInProgress NOTIFY downloadInProgressChanged)
  // Like the Android downloader: only maps with something downloaded, or the maps left to download,
  // with the regions around the current position first at the root.
  Q_PROPERTY(bool downloadedOnly READ downloadedOnly WRITE setDownloadedOnly NOTIFY downloadedOnlyChanged)
  // Searches all maps by name when set; leading and trailing spaces are ignored.
  Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
  // Downloaded maps with a newer version, and the download size of all their updates.
  Q_PROPERTY(int updateCount READ updateCount NOTIFY updatesChanged)
  Q_PROPERTY(QString updateSize READ updateSize NOTIFY updatesChanged)
  // The status of the parent node, for "Download All" in a group like on Android.
  Q_PROPERTY(int parentStatus READ parentStatus NOTIFY downloadInProgressChanged)
  // The map being downloaded and its progress (0..1), for the download notification.
  Q_PROPERTY(QString downloadingName READ downloadingName NOTIFY downloadingChanged)
  Q_PROPERTY(double downloadingProgress READ downloadingProgress NOTIFY downloadingChanged)

public:
  // Mirrors storage::NodeStatus for QML.
  enum Status
  {
    Undefined,
    Downloading,
    Applying,
    InQueue,
    Error,
    OnDiskOutOfDate,
    OnDisk,
    NotDownloaded,
    Partly,
  };
  Q_ENUM(Status)

  // Mirrors storage::NodeErrorCode for QML.
  enum ErrorCode
  {
    NoError,
    UnknownError,
    OutOfMemFailed,
    NoInetConnection,
  };
  Q_ENUM(ErrorCode)

  enum Roles
  {
    CountryIdRole = Qt::UserRole + 1,
    NameRole,
    IsGroupRole,
    StatusRole,
    ErrorRole,
    SizeRole,
    LocalSizeRole,
    ProgressRole,
    MapsCountRole,
    LocalMapsCountRole,
    DescriptionRole,
    // List section: "Near me", the first letter, or empty.
    SectionRole,
    // Search results: the matched name and the country it belongs to.
    FoundNameRole,
    ParentNameRole,
  };

  explicit CountriesModel(QObject * parent = nullptr);
  ~CountriesModel() override;

  QString parentId() const;
  void setParentId(QString const & parentId);
  QString title() const;
  bool downloadInProgress() const;
  bool downloadedOnly() const { return m_downloadedOnly; }
  void setDownloadedOnly(bool downloadedOnly);
  QString query() const { return m_query; }
  void setQuery(QString const & query);

  int rowCount(QModelIndex const & parent = QModelIndex()) const override;
  QVariant data(QModelIndex const & index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  Q_INVOKABLE void download(QString const & countryId);
  Q_INVOKABLE void cancel(QString const & countryId);
  Q_INVOKABLE void remove(QString const & countryId);
  Q_INVOKABLE void update(QString const & countryId);
  Q_INVOKABLE void retry(QString const & countryId);
  Q_INVOKABLE void showOnMap(QString const & countryId);
  // Updates all outdated maps, like "Update all" on Android.
  Q_INVOKABLE void updateAll();
  // Cancels all downloads, like Cancel in the Android download notification.
  Q_INVOKABLE void cancelAll();
  // Checks before downloading, updating and deleting, like MapManagerHelper and DownloaderAdapter on Android.
  Q_INVOKABLE bool hasSpaceToDownload(QString const & countryId) const;
  Q_INVOKABLE bool hasSpaceToUpdate(QString const & countryId) const;
  Q_INVOKABLE bool hasUnsavedEdits(QString const & countryId) const;
  Q_INVOKABLE bool navigating() const;
  // There are updates the user wasn't asked about yet, like the Android map update dialog after an app update.
  Q_INVOKABLE bool shouldOfferUpdate() const;
  Q_INVOKABLE void setUpdateOffered();

  int updateCount() const;
  QString updateSize() const;
  int parentStatus() const;
  QString downloadingName() const { return m_downloadingName; }
  double downloadingProgress() const { return m_downloadingProgress; }

signals:
  void parentIdChanged();
  void downloadInProgressChanged();
  void downloadedOnlyChanged();
  void queryChanged();
  void updatesChanged();
  void downloadingChanged();
  void downloadFailed(QString const & name);

private:
  void Reload();
  void SetChildren(storage::CountriesVec && children);
  void OnCountryChanged(storage::CountryId const & countryId);
  void OnProgress(storage::CountryId const & countryId, downloader::Progress const & progress);

  storage::Storage & m_storage;
  storage::CountryId m_parentId;
  storage::CountriesVec m_children;
  // Row index from which the rows are no longer "Near me" ones.
  size_t m_nearCount = 0;
  std::vector<std::string> m_foundNames;
  bool m_downloadedOnly = false;
  QString m_query;
  uint64_t m_searchTimestamp = 0;
  int m_slotId = 0;
  bool m_downloadInProgress = false;
  QString m_downloadingName;
  double m_downloadingProgress = 0;
};
}  // namespace sailfish
