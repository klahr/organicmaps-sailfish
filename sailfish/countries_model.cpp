#include "sailfish/countries_model.hpp"

#include "sailfish/app_info.hpp"
#include "sailfish/framework_access.hpp"

#include "map/framework.hpp"
#include "map/search_api.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/downloader_search_params.hpp"
#include "storage/storage.hpp"
#include "storage/storage_helpers.hpp"

#include "platform/settings.hpp"

#include <QCollator>
#include <QPointer>

#include <algorithm>

namespace sailfish
{
namespace
{
using storage::NodeErrorCode;
using storage::NodeStatus;

// The data version the update was last offered for.
std::string_view constexpr kUpdateOfferedSetting = "SailfishUpdateOfferedVersion";

static_assert(CountriesModel::Undefined == static_cast<int>(NodeStatus::Undefined));
static_assert(CountriesModel::Downloading == static_cast<int>(NodeStatus::Downloading));
static_assert(CountriesModel::Applying == static_cast<int>(NodeStatus::Applying));
static_assert(CountriesModel::InQueue == static_cast<int>(NodeStatus::InQueue));
static_assert(CountriesModel::Error == static_cast<int>(NodeStatus::Error));
static_assert(CountriesModel::OnDiskOutOfDate == static_cast<int>(NodeStatus::OnDiskOutOfDate));
static_assert(CountriesModel::OnDisk == static_cast<int>(NodeStatus::OnDisk));
static_assert(CountriesModel::NotDownloaded == static_cast<int>(NodeStatus::NotDownloaded));
static_assert(CountriesModel::Partly == static_cast<int>(NodeStatus::Partly));
static_assert(CountriesModel::NoError == static_cast<int>(NodeErrorCode::NoError));
static_assert(CountriesModel::UnknownError == static_cast<int>(NodeErrorCode::UnknownError));
static_assert(CountriesModel::OutOfMemFailed == static_cast<int>(NodeErrorCode::OutOfMemFailed));
static_assert(CountriesModel::NoInetConnection == static_cast<int>(NodeErrorCode::NoInetConnection));

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
  m_slotId = m_storage.Subscribe([this](storage::CountryId const & countryId) {
    OnCountryChanged(countryId);
  }, [this](storage::CountryId const & countryId, downloader::Progress const &) { OnCountryChanged(countryId); });
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

void CountriesModel::setDownloadedOnly(bool downloadedOnly)
{
  if (downloadedOnly == m_downloadedOnly)
    return;
  m_downloadedOnly = downloadedOnly;
  Reload();
  emit downloadedOnlyChanged();
}

void CountriesModel::setQuery(QString const & query)
{
  QString const trimmed = query.trimmed();
  if (trimmed == m_query)
    return;
  m_query = trimmed;
  emit queryChanged();
  Reload();
}

void CountriesModel::Reload()
{
  ++m_searchTimestamp;
  m_nearCount = 0;
  if (!m_query.isEmpty())
  {
    // Same request as the Android downloader search; results arrive on the GUI thread.
    QPointer<CountriesModel> self(this);
    auto const timestamp = m_searchTimestamp;
    storage::DownloaderSearchParams params{m_query.toStdString(), GetInputLocale(),
                                           [self, timestamp](storage::DownloaderSearchResults results)
    {
      if (!self || timestamp != self->m_searchTimestamp)
        return;
      storage::CountriesVec ids;
      std::vector<std::string> names;
      for (auto const & r : results.m_results)
      {
        if (std::find(ids.begin(), ids.end(), r.m_countryId) == ids.end())
        {
          ids.push_back(r.m_countryId);
          names.push_back(r.m_matchedName);
        }
      }
      self->beginResetModel();
      self->m_children = std::move(ids);
      self->m_foundNames = std::move(names);
      self->endResetModel();
    }};
    GetFramework().GetSearchAPI().SearchInDownloader(std::move(params));
    return;
  }

  m_foundNames.clear();
  storage::CountriesVec downloaded, available;
  m_storage.GetChildrenInGroups(m_parentId, downloaded, available, true /* keepAvailableChildren */);
  SetChildren(m_downloadedOnly ? std::move(downloaded) : std::move(available));
}

void CountriesModel::SetChildren(storage::CountriesVec && children)
{
  // countries.txt is ordered by id; list by the localized name instead.
  QCollator collator;
  std::vector<std::pair<QString, storage::CountryId>> named;
  named.reserve(children.size());
  for (auto & id : children)
    named.emplace_back(QString::fromStdString(GetAttrs(m_storage, id).m_nodeLocalName), std::move(id));
  std::sort(named.begin(), named.end(),
            [&collator](auto const & lhs, auto const & rhs) { return collator.compare(lhs.first, rhs.first) < 0; });

  storage::CountriesVec result;
  // Regions around the position come first in the list of maps to download, as on Android.
  auto & framework = GetFramework();
  if (!m_downloadedOnly && m_parentId == m_storage.GetRootId())
  {
    if (auto const position = framework.GetCurrentPosition())
    {
      storage::CountriesVec near;
      framework.GetCountryInfoGetter().GetRegionsCountryId(*position, near);
      for (auto const & id : near)
        if (!GetAttrs(m_storage, id).m_present)
          result.push_back(id);
    }
  }
  m_nearCount = result.size();
  for (auto & item : named)
    result.push_back(std::move(item.second));

  beginResetModel();
  m_children = std::move(result);
  endResetModel();
}

void CountriesModel::OnCountryChanged(storage::CountryId const & countryId)
{
  // A map that finished downloading or was deleted moves between the downloaded and available lists.
  auto const status = GetAttrs(m_storage, countryId).m_status;
  if (m_query.isEmpty() && (status == NodeStatus::OnDisk || status == NodeStatus::NotDownloaded))
  {
    Reload();
    return;
  }

  // A leaf change also changes every group above it, and the lists are short, so refresh all rows.
  if (!m_children.empty())
    emit dataChanged(index(0), index(static_cast<int>(m_children.size()) - 1));

  bool const inProgress = m_storage.IsDownloadInProgress();
  if (inProgress != m_downloadInProgress)
  {
    m_downloadInProgress = inProgress;
    emit downloadInProgressChanged();
  }
  emit updatesChanged();
}

int CountriesModel::updateCount() const
{
  storage::Storage::UpdateInfo info;
  return m_storage.GetUpdateInfo(m_storage.GetRootId(), info) ? static_cast<int>(info.m_numberOfMwmFilesToUpdate) : 0;
}

QString CountriesModel::updateSize() const
{
  storage::Storage::UpdateInfo info;
  if (!m_storage.GetUpdateInfo(m_storage.GetRootId(), info))
    return {};
  return FormatSize(static_cast<qint64>(info.m_totalDownloadSizeInBytes));
}

void CountriesModel::updateAll()
{
  m_storage.UpdateNode(m_storage.GetRootId());
}

bool CountriesModel::shouldOfferUpdate() const
{
  int64_t offered = 0;
  settings::TryGet(kUpdateOfferedSetting, offered);
  return updateCount() > 0 && offered != m_storage.GetCurrentDataVersion() &&
         storage::IsEnoughSpaceForUpdate(m_storage.GetRootId(), m_storage);
}

void CountriesModel::setUpdateOffered()
{
  settings::Set(kUpdateOfferedSetting, m_storage.GetCurrentDataVersion());
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
  case DescriptionRole: return QString::fromStdString(attrs.m_nodeLocalDescription);
  case SectionRole:
    if (!m_query.isEmpty())
      return QString();
    if (m_downloadedOnly)
      return Localized("downloader_downloaded_subtitle");
    if (static_cast<size_t>(index.row()) < m_nearCount)
      return Localized("downloader_near_me_subtitle");
    return QString::fromStdString(attrs.m_nodeLocalName).left(1).toUpper();
  case FoundNameRole:
    return static_cast<size_t>(index.row()) < m_foundNames.size()
             ? QString::fromStdString(m_foundNames[static_cast<size_t>(index.row())])
             : QString();
  case ParentNameRole:
    return attrs.m_topmostParentInfo.empty() ? QString()
                                             : QString::fromStdString(attrs.m_topmostParentInfo.front().m_localName);
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
      {CountryIdRole, "countryId"},
      {NameRole, "name"},
      {IsGroupRole, "isGroup"},
      {StatusRole, "status"},
      {ErrorRole, "error"},
      {SizeRole, "size"},
      {LocalSizeRole, "localSize"},
      {ProgressRole, "progress"},
      {MapsCountRole, "mapsCount"},
      {LocalMapsCountRole, "localMapsCount"},
      {DescriptionRole, "description"},
      {SectionRole, "section"},
      {FoundNameRole, "foundName"},
      {ParentNameRole, "parentName"},
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
}  // namespace sailfish
