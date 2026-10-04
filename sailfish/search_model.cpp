#include "sailfish/search_model.hpp"

#include "sailfish/app_info.hpp"
#include "sailfish/app_settings.hpp"
#include "sailfish/countries_model.hpp"
#include "sailfish/framework_access.hpp"

#include "map/everywhere_search_params.hpp"
#include "map/framework.hpp"
#include "map/search_api.hpp"
#include "map/viewport_search_params.hpp"

#include "search/displayed_categories.hpp"
#include "search/result.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/storage.hpp"

#include "platform/distance.hpp"

#include "geometry/mercator.hpp"

#include <QPointer>

#include <utility>

namespace sailfish
{
namespace
{
SearchModel::OpenState GetOpenState(search::Result const & result)
{
  switch (result.IsOpenNow())
  {
  case osm::Yes: return result.GetMinutesUntilClosed() < 60 ? SearchModel::ClosingSoon : SearchModel::Open;
  case osm::No: return result.GetMinutesUntilOpen() < 60 ? SearchModel::OpeningSoon : SearchModel::Closed;
  case osm::Unknown: return SearchModel::OpenUnknown;
  }
  return SearchModel::OpenUnknown;
}

// Wraps the highlight ranges, which index the UTF-16 text like on the other Qt and Java frontends, into
// colored styled text.
template <typename RangeFn>
QString Highlighted(std::string const & text, size_t rangesCount, RangeFn && range, QColor const & color)
{
  QString const str = QString::fromStdString(text);
  QString const open = QStringLiteral("<font color=\"%1\">").arg(color.name());
  QString styled;
  int pos = 0;
  for (size_t i = 0; i < rangesCount; ++i)
  {
    auto const & [first, length] = range(i);
    styled += str.mid(pos, first - pos).toHtmlEscaped();
    styled += open + str.mid(first, length).toHtmlEscaped() + QStringLiteral("</font>");
    pos = first + length;
  }
  return styled + str.mid(pos).toHtmlEscaped();
}
}  // namespace

SearchModel::SearchModel(QObject * parent)
  : QAbstractListModel(parent)
  , m_framework(GetFramework())
  , m_locale(GetInputLocale())
  , m_results(std::make_unique<search::Results>())
{
  m_storageSlot = m_framework.GetStorage().Subscribe([this](storage::CountryId const &)
  { emit mapsChanged(); }, [this](storage::CountryId const &, downloader::Progress const &) { emit mapsChanged(); });
}

SearchModel::~SearchModel()
{
  m_framework.GetStorage().Unsubscribe(m_storageSlot);
  // Also clears the viewport search marks.
  m_framework.GetSearchAPI().CancelAllSearches();
}

int SearchModel::rowCount(QModelIndex const & parent) const
{
  return parent.isValid() ? 0 : static_cast<int>(Results().GetCount());
}

QVariant SearchModel::data(QModelIndex const & index, int role) const
{
  if (!index.isValid() || index.row() >= rowCount())
    return {};

  auto const & result = Results()[static_cast<size_t>(index.row())];
  bool const isFeature = result.GetResultType() == search::Result::Type::Feature;
  switch (role)
  {
  case NameRole:
    // Unnamed places are titled by their type, as on Android.
    if (result.GetString().empty() && isFeature)
      return QString::fromStdString(result.GetLocalizedFeatureType()).toHtmlEscaped();
    return Highlighted(result.GetString(), result.GetHighlightRangesCount(),
                       [&](size_t i) { return result.GetHighlightRange(i); }, m_highlightColor);
  case DescriptionRole:
    return isFeature ? QString::fromStdString(result.GetFeatureDescription(result.GetLocalizedFeatureType()))
                     : QString();
  case AddressRole:
    return Highlighted(result.GetAddress(), result.GetDescHighlightRangesCount(),
                       [&](size_t i) { return result.GetDescHighlightRange(i); }, m_highlightColor);
  case DistanceRole:
  {
    auto const position = m_framework.GetCurrentPosition();
    if (!position || !result.HasPoint() || result.IsSuggest())
      return QString();
    return QString::fromStdString(
        platform::Distance::CreateFormatted(mercator::DistanceOnEarth(*position, result.GetFeatureCenter()))
            .ToString());
  }
  case OpenStatusRole:
    switch (GetOpenState(result))
    {
    case Open: return Localized("editor_time_open");
    case ClosingSoon: return Localized("closes_in", {FormatDuration(result.GetMinutesUntilClosed() * 60L)});
    case OpeningSoon: return Localized("opens_in", {FormatDuration(result.GetMinutesUntilOpen() * 60L)});
    case Closed: return Localized("closed");
    case OpenUnknown: return QString();
    }
    return QString();
  case OpenStateRole: return GetOpenState(result);
  case SuggestRole: return result.IsSuggest();
  default: return {};
  }
}

QHash<int, QByteArray> SearchModel::roleNames() const
{
  return {{NameRole, "name"},         {DescriptionRole, "description"}, {AddressRole, "address"},
          {DistanceRole, "distance"}, {OpenStatusRole, "openStatus"},   {OpenStateRole, "openState"},
          {SuggestRole, "suggest"}};
}

void SearchModel::setQuery(QString const & query)
{
  if (query == m_query)
    return;
  m_query = query;
  emit queryChanged();
  Run();
}

QVariantList SearchModel::categories() const
{
  QVariantList categories;
  auto const & displayed = m_framework.GetDisplayedCategories();
  for (auto const & key : displayed.GetKeys())
  {
    // The first synonym in the search language is the category name, with English as a fallback.
    std::string name, english;
    displayed.ForEachSynonym(key, [&](std::string const & synonym, std::string const & locale)
    {
      if (name.empty() && locale == m_locale)
        name = synonym;
      if (english.empty() && locale == "en")
        english = synonym;
    });
    QVariantMap category;
    category["key"] = QString::fromStdString(key);
    category["name"] = QString::fromStdString(name.empty() ? english : name);
    categories.append(category);
  }
  return categories;
}

void SearchModel::searchCategory(QString const & name, bool addToHistory)
{
  // The trailing space tells the search that the category name is complete.
  m_categoryQuery = name + ' ';
  setQuery(m_categoryQuery);
  if (addToHistory)
    SaveToHistory(name);
}

bool SearchModel::activate(int row)
{
  if (row < 0 || row >= rowCount())
    return false;

  auto const & result = Results()[static_cast<size_t>(row)];
  if (result.IsSuggest())
  {
    setQuery(QString::fromStdString(result.GetSuggestionString()));
    return false;
  }
  SaveToHistory(m_query);
  m_framework.SelectSearchResult(result, true /* animation */);
  return true;
}

void SearchModel::showOnMap()
{
  if (Results().GetCount() == 0)
    return;
  SaveToHistory(m_query);
  m_framework.UpdateViewport(Results());
}

QStringList SearchModel::history() const
{
  QStringList history;
  for (auto const & request : m_framework.GetSearchAPI().GetLastSearchQueries())
    history.append(QString::fromStdString(request.second));
  return history;
}

bool SearchModel::noMaps() const
{
  return m_framework.GetStorage().GetDownloadedFilesCount() == 0;
}

QVariantMap SearchModel::suggestedMap() const
{
  auto const position = m_framework.GetCurrentPosition();
  if (!position)
    return {};
  return MissingMapInfo(m_framework.GetStorage(), m_framework.GetCountryInfoGetter().GetRegionCountryId(*position));
}

void SearchModel::downloadSuggestedMap()
{
  auto const id = suggestedMap().value("countryId").toString().toStdString();
  if (!id.empty())
    DownloadMap(m_framework.GetStorage(), id);
}

void SearchModel::clearHistory()
{
  m_framework.GetSearchAPI().ClearSearchHistory();
  emit historyChanged();
}

void SearchModel::SaveToHistory(QString const & query)
{
  QString const trimmed = query.trimmed();
  if (trimmed.isEmpty() || !AppSettings::IsSearchHistoryEnabled())
    return;
  m_framework.GetSearchAPI().SaveSearchQuery({m_locale, trimmed.toStdString()});
  emit historyChanged();
}

void SearchModel::Run()
{
  auto const timestamp = ++m_timestamp;
  beginResetModel();
  Results().Clear();
  endResetModel();

  auto & api = m_framework.GetSearchAPI();
  if (m_query.isEmpty())
  {
    api.CancelAllSearches();
    SetSearching(false);
    return;
  }

  bool const isCategory = m_query == m_categoryQuery;
  // As on Android, the viewport search draws the result marks and SearchAPI repeats it whenever the map
  // moves, until the query is cleared.
  api.SearchInViewport(
      {m_query.toStdString(), m_locale, {} /* timeout */, isCategory, {} /* onStarted */, {} /* onCompleted */});

  // Results arrive on the GUI thread and may outlive the page that owns the model.
  QPointer<SearchModel> self(this);
  search::EverywhereSearchParams params{m_query.toStdString(),
                                        m_locale,
                                        {} /* timeout */,
                                        isCategory,
                                        [self, timestamp](search::Results results)
  {
    if (self)
      self->OnResults(timestamp, std::move(results));
  }};
  SetSearching(api.SearchEverywhere(std::move(params)));
}

void SearchModel::OnResults(uint64_t timestamp, search::Results && results)
{
  if (timestamp != m_timestamp)
    return;

  beginResetModel();
  *m_results = std::move(results);
  endResetModel();
  if (Results().IsEndMarker())
    SetSearching(false);
}

void SearchModel::SetSearching(bool searching)
{
  if (searching == m_searching)
    return;
  m_searching = searching;
  emit searchingChanged();
}

}  // namespace sailfish
