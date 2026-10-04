#pragma once

#include <QAbstractListModel>
#include <QColor>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include <cstdint>
#include <memory>

class Framework;

// Qt 5.6 moc can't parse the search headers, so the results are kept behind a pointer.
namespace search
{
class Result;
class Results;
}  // namespace search

namespace sailfish
{
// Everywhere search for the result list plus a viewport search that keeps the map marks up to date.
class SearchModel : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
  Q_PROPERTY(bool searching READ searching NOTIFY searchingChanged)
  // Recent queries, newest first.
  Q_PROPERTY(QStringList history READ history NOTIFY historyChanged)
  // Color of the matched parts of result names, which are returned as styled text.
  Q_PROPERTY(QColor highlightColor MEMBER m_highlightColor)
  // No map is downloaded yet: search offers the map of the position, like CountrySuggestFragment on Android.
  Q_PROPERTY(bool noMaps READ noMaps NOTIFY mapsChanged)
  // That map, see MissingMapInfo(); empty without a position.
  Q_PROPERTY(QVariantMap suggestedMap READ suggestedMap NOTIFY mapsChanged)

public:
  enum Roles
  {
    NameRole = Qt::UserRole + 1,
    DescriptionRole,
    AddressRole,
    DistanceRole,
    OpenStatusRole,
    OpenStateRole,
    // A completion of the query rather than a place.
    SuggestRole
  };

  // Opening hours state of a result, colored like on Android.
  enum OpenState
  {
    OpenUnknown,
    Open,
    ClosingSoon,
    OpeningSoon,
    Closed
  };
  Q_ENUM(OpenState)

  explicit SearchModel(QObject * parent = nullptr);
  ~SearchModel() override;

  int rowCount(QModelIndex const & parent = {}) const override;
  QVariant data(QModelIndex const & index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  QString query() const { return m_query; }
  void setQuery(QString const & query);
  bool searching() const { return m_searching; }
  QStringList history() const;
  bool noMaps() const;
  QVariantMap suggestedMap() const;

  // Displayed search categories as {key, name} in the search language.
  Q_INVOKABLE QVariantList categories() const;
  // The navigation quick search leaves the history alone, like on Android.
  Q_INVOKABLE void searchCategory(QString const & name, bool addToHistory = true);
  // Returns false for a suggestion, which replaces the query instead of selecting a place.
  Q_INVOKABLE bool activate(int row);
  // Fits the viewport to the results, like the search key on Android.
  Q_INVOKABLE void showOnMap();
  Q_INVOKABLE void clearHistory();
  Q_INVOKABLE void downloadSuggestedMap();

signals:
  void queryChanged();
  void searchingChanged();
  void historyChanged();
  void mapsChanged();

private:
  void Run();
  void OnResults(uint64_t timestamp, search::Results && results);
  search::Results & Results() { return *m_results; }
  search::Results const & Results() const { return *m_results; }
  void SetSearching(bool searching);
  // Same moments as on Android: a chosen result, a category, or showing all results on the map.
  void SaveToHistory(QString const & query);

  Framework & m_framework;
  std::string const m_locale;
  QString m_query;
  QString m_categoryQuery;
  std::unique_ptr<search::Results> m_results;
  uint64_t m_timestamp = 0;
  bool m_searching = false;
  QColor m_highlightColor;
  int m_storageSlot = 0;
};
}  // namespace sailfish
