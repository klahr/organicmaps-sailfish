#pragma once

#include "storage/storage_defines.hpp"

#include <QAbstractListModel>
#include <QString>

#include <vector>

namespace storage
{
class Storage;
}

namespace sailfish
{
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
  // Searches all maps by name when set.
  Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)

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
  Q_INVOKABLE static QString formatSize(qint64 bytes);

signals:
  void parentIdChanged();
  void downloadInProgressChanged();
  void downloadedOnlyChanged();
  void queryChanged();

private:
  void Reload();
  void SetChildren(storage::CountriesVec && children);
  void OnCountryChanged(storage::CountryId const & countryId);

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
};
}  // namespace sailfish
