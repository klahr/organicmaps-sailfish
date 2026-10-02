#include "sailfish/countries_model.hpp"

#include "sailfish/framework_access.hpp"

#include "map/framework.hpp"

#include "storage/storage.hpp"

#include <QCollator>

#include <algorithm>

namespace sailfish
{
namespace
{
storage::NodeAttrs GetAttrs(storage::Storage const & storage, storage::CountryId const & countryId)
{
  storage::NodeAttrs attrs;
  storage.GetNodeAttrs(countryId, attrs);
  return attrs;
}

storage::CountryId ToCountryId(QString const & countryId)
{
  return countryId.toStdString();
}
}  // namespace

CountriesModel::CountriesModel(QObject * parent) : QAbstractListModel(parent), m_storage(GetFramework().GetStorage())
{
  m_parentId = m_storage.GetRootId();
  m_downloadInProgress = m_storage.IsDownloadInProgress();
  m_slotId = m_storage.Subscribe([this](storage::CountryId const & countryId) { OnCountryChanged(countryId); },
                                 [this](storage::CountryId const & countryId, downloader::Progress const &)
  { OnCountryChanged(countryId); });
  Reload();
}

CountriesModel::~CountriesModel()
{
  m_storage.Unsubscribe(m_slotId);
}

QString CountriesModel::parentId() const
{
  return QString::fromStdString(m_parentId);
}

void CountriesModel::setParentId(QString const & parentId)
{
  storage::CountryId const id = parentId.isEmpty() ? m_storage.GetRootId() : ToCountryId(parentId);
  if (id == m_parentId)
    return;

  m_parentId = id;
  Reload();
  emit parentIdChanged();
}

QString CountriesModel::title() const
{
  return QString::fromStdString(GetAttrs(m_storage, m_parentId).m_nodeLocalName);
}

bool CountriesModel::downloadInProgress() const
{
  return m_downloadInProgress;
}

void CountriesModel::Reload()
{
  beginResetModel();
  m_children.clear();
  m_storage.GetChildren(m_parentId, m_children);

  // countries.txt is ordered by id; list by the localized name instead.
  QCollator collator;
  std::vector<std::pair<QString, storage::CountryId>> named;
  named.reserve(m_children.size());
  for (auto const & id : m_children)
    named.emplace_back(QString::fromStdString(GetAttrs(m_storage, id).m_nodeLocalName), id);
  std::sort(named.begin(), named.end(),
            [&collator](auto const & lhs, auto const & rhs) { return collator.compare(lhs.first, rhs.first) < 0; });
  for (size_t i = 0; i < named.size(); ++i)
    m_children[i] = std::move(named[i].second);

  endResetModel();
}

void CountriesModel::OnCountryChanged(storage::CountryId const &)
{
  // A leaf change also changes every group above it, and the lists are short, so refresh all rows.
  if (!m_children.empty())
    emit dataChanged(index(0), index(static_cast<int>(m_children.size()) - 1));

  bool const inProgress = m_storage.IsDownloadInProgress();
  if (inProgress != m_downloadInProgress)
  {
    m_downloadInProgress = inProgress;
    emit downloadInProgressChanged();
  }
}

int CountriesModel::rowCount(QModelIndex const & parent) const
{
  return parent.isValid() ? 0 : static_cast<int>(m_children.size());
}

QVariant CountriesModel::data(QModelIndex const & index, int role) const
{
  if (!index.isValid() || index.row() >= static_cast<int>(m_children.size()))
    return {};

  storage::CountryId const & countryId = m_children[index.row()];
  if (role == CountryIdRole)
    return QString::fromStdString(countryId);
  if (role == IsGroupRole)
    return !m_storage.IsLeaf(countryId);

  auto const attrs = GetAttrs(m_storage, countryId);
  switch (role)
  {
  case NameRole: return QString::fromStdString(attrs.m_nodeLocalName);
  case StatusRole: return static_cast<int>(attrs.m_status);
  case ErrorRole: return static_cast<int>(attrs.m_error);
  case SizeRole: return static_cast<qint64>(attrs.m_mwmSize);
  case LocalSizeRole: return static_cast<qint64>(attrs.m_localMwmSize);
  case MapsCountRole: return static_cast<int>(attrs.m_mwmCounter);
  case LocalMapsCountRole: return static_cast<int>(attrs.m_localMwmCounter);
  case ProgressRole:
  {
    auto const & progress = attrs.m_downloadingProgress;
    if (progress.IsUnknown() || progress.m_bytesTotal <= 0)
      return 0.0;
    return static_cast<double>(progress.m_bytesDownloaded) / progress.m_bytesTotal;
  }
  default: return {};
  }
}

QHash<int, QByteArray> CountriesModel::roleNames() const
{
  return {
      {CountryIdRole, "countryId"},       {NameRole, "name"},
      {IsGroupRole, "isGroup"},           {StatusRole, "status"},
      {ErrorRole, "error"},               {SizeRole, "size"},
      {LocalSizeRole, "localSize"},       {ProgressRole, "progress"},
      {MapsCountRole, "mapsCount"},       {LocalMapsCountRole, "localMapsCount"},
  };
}

void CountriesModel::download(QString const & countryId)
{
  m_storage.DownloadNode(ToCountryId(countryId));
}

void CountriesModel::cancel(QString const & countryId)
{
  m_storage.CancelDownloadNode(ToCountryId(countryId));
}

void CountriesModel::remove(QString const & countryId)
{
  m_storage.DeleteNode(ToCountryId(countryId));
}

void CountriesModel::update(QString const & countryId)
{
  m_storage.UpdateNode(ToCountryId(countryId));
}

void CountriesModel::retry(QString const & countryId)
{
  m_storage.RetryDownloadNode(ToCountryId(countryId));
}

void CountriesModel::showOnMap(QString const & countryId)
{
  GetFramework().ShowNode(ToCountryId(countryId));
}

// static
QString CountriesModel::formatSize(qint64 bytes)
{
  qint64 constexpr kMb = 1024 * 1024;
  if (bytes >= kMb)
    return QStringLiteral("%1 MB").arg((bytes + kMb / 2) / kMb);
  return QStringLiteral("%1 kB").arg((bytes + 1023) / 1024);
}
}  // namespace sailfish
