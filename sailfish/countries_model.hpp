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
  };

  explicit CountriesModel(QObject * parent = nullptr);
  ~CountriesModel() override;

  QString parentId() const;
  void setParentId(QString const & parentId);
  QString title() const;
  bool downloadInProgress() const;

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

private:
  void Reload();
  void OnCountryChanged(storage::CountryId const & countryId);

  storage::Storage & m_storage;
  storage::CountryId m_parentId;
  storage::CountriesVec m_children;
  int m_slotId = 0;
  bool m_downloadInProgress = false;
};
}  // namespace sailfish
