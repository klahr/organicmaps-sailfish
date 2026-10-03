#pragma once

#include <QMediaPlayer>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

class Framework;

namespace sailfish
{
class VoiceGuide;

// Route planning like the Android RoutingController: route points from the place page, the router
// type, building and its result. Kept free of map headers, which Qt 5.6 moc can't parse.
class Routing : public QObject
{
  Q_OBJECT
  // The route panel is shown while there are route points.
  Q_PROPERTY(bool active READ active NOTIFY pointsChanged)
  // routing::RouterType: Vehicle, Pedestrian, Bicycle, Transit, Ruler.
  Q_PROPERTY(int routerType READ routerType WRITE setRouterType NOTIFY routerTypeChanged)
  // Route points in order as {type, title, subtitle, isMyPosition}; type is RouteMarkType.
  Q_PROPERTY(QVariantList points READ points NOTIFY pointsChanged)
  Q_PROPERTY(bool building READ building NOTIFY stateChanged)
  Q_PROPERTY(bool built READ built NOTIFY stateChanged)
  // "11 min • 4.4 km", or "Distance: 2.1 km" for the ruler, like on Android.
  Q_PROPERTY(QString summary READ summary NOTIFY stateChanged)
  // Transit routes: the walking part of the summary, and the legs as {icon, number, color} chips.
  Q_PROPERTY(QString walkingDistance READ walkingDistance NOTIFY stateChanged)
  Q_PROPERTY(QVariantList transitSteps READ transitSteps NOTIFY stateChanged)
  // Why the route could not be built, with the Android wording.
  Q_PROPERTY(QString errorTitle READ errorTitle NOTIFY stateChanged)
  Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)
  // Maps to download before the route can be built.
  Q_PROPERTY(QStringList missingMaps READ missingMaps NOTIFY stateChanged)
  Q_PROPERTY(QString missingMapsSize READ missingMapsSize NOTIFY stateChanged)
  // Routing options: a RoutingOptions::Road mask of avoided roads, and stop reordering.
  Q_PROPERTY(int avoidRoads READ avoidRoads WRITE setAvoidRoads NOTIFY optionsChanged)
  Q_PROPERTY(bool routeOptimization READ routeOptimization WRITE setRouteOptimization NOTIFY optionsChanged)
  // START is offered for built car, walking and bicycle routes, as on Android.
  Q_PROPERTY(bool canStart READ canStart NOTIFY stateChanged)
  Q_PROPERTY(bool navigating READ navigating NOTIFY navigationChanged)
  // The Android navigation panels: turnIcon, distanceToTurn, street, nextTurnIcon, hoursLeft, minutesLeft,
  // hourUnits, minuteUnits, arrival, distanceLeftValue, distanceLeftUnits, speed, speedLimit, speedCamLimitExceeded,
  // lanes as {icon, active} and progress (0..1).
  Q_PROPERTY(QVariantMap navigation READ navigation NOTIFY navigationChanged)
  // Voice instructions while navigating, see VoiceGuide: there is a voice for some language.
  Q_PROPERTY(bool voiceAvailable READ voiceAvailable NOTIFY voiceChanged)
  Q_PROPERTY(bool voiceEnabled READ voiceEnabled WRITE setVoiceEnabled NOTIFY voiceChanged)
  // The spoken language, a code of voiceLanguages; setting it chooses it over the app language.
  Q_PROPERTY(QString voiceLanguage READ voiceLanguage WRITE setVoiceLanguage NOTIFY voiceChanged)
  // Its name and its index in voiceLanguages, -1 without a voice.
  Q_PROPERTY(QString voiceLanguageName READ voiceLanguageName NOTIFY voiceChanged)
  Q_PROPERTY(int voiceLanguageIndex READ voiceLanguageIndex NOTIFY voiceChanged)
  // Languages with a voice as {code, name, speechNote}.
  Q_PROPERTY(QVariantList voiceLanguages READ voiceLanguages NOTIFY voiceChanged)
  // Speech Note speaks with natural voices; without a voice for a language it offers to get one.
  Q_PROPERTY(bool speechNoteInstalled READ speechNoteInstalled NOTIFY voiceChanged)
  // The language voice instructions would ideally be in: the chosen or the app language.
  Q_PROPERTY(QString wantedVoiceLanguageName READ wantedVoiceLanguageName NOTIFY voiceChanged)
  Q_PROPERTY(bool wantedHasSpeechNoteVoice READ wantedHasSpeechNoteVoice NOTIFY voiceChanged)
  Q_PROPERTY(bool announceStreets READ announceStreets WRITE setAnnounceStreets NOTIFY voiceChanged)

public:
  // Mirrors routing::RouterType.
  enum RouterType
  {
    Vehicle,
    Pedestrian,
    Bicycle,
    Transit,
    Ruler
  };
  Q_ENUM(RouterType)

  // Mirrors RouteMarkType.
  enum PointType
  {
    Start,
    Intermediate,
    Finish
  };
  Q_ENUM(PointType)

  // RoutingOptions::Road values.
  enum Road
  {
    Toll = 1 << 1,
    Motorway = 1 << 2,
    Ferry = 1 << 3,
    Dirty = 1 << 4
  };
  Q_ENUM(Road)

  explicit Routing(Framework & framework, QObject * parent = nullptr);
  ~Routing() override;

  bool active() const;
  int routerType() const;
  void setRouterType(int type);
  QVariantList points() const;
  bool building() const { return m_building; }
  bool built() const { return m_built; }
  QString summary() const { return m_summary; }
  QString walkingDistance() const { return m_walkingDistance; }
  QVariantList transitSteps() const { return m_transitSteps; }
  QString errorTitle() const { return m_errorTitle; }
  QString errorMessage() const { return m_errorMessage; }
  QStringList missingMaps() const { return m_missingMaps; }
  QString missingMapsSize() const;
  int avoidRoads() const;
  void setAvoidRoads(int roads);
  bool routeOptimization() const;
  void setRouteOptimization(bool enabled);
  bool canStart() const;
  bool navigating() const { return m_navigating; }
  QVariantMap navigation() const { return m_navigation; }

  // Called on every location update to refresh the navigation panels.
  void UpdateNavigation(double speedMps);

  // Uses the place shown in the place page, like its Route from / Route to / Add stop buttons.
  Q_INVOKABLE void routeFromPlace();
  Q_INVOKABLE void routeToPlace();
  Q_INVOKABLE void addStopFromPlace();
  Q_INVOKABLE void setStartToMyPosition();
  // Replaces the route with points given as {lat, lon, name}, e.g. by an om:// route link; the first is the
  // start and the last the finish.
  Q_INVOKABLE void planRoute(int routerType, QVariantList const & points);
  Q_INVOKABLE void removePoint(int index);
  Q_INVOKABLE void movePoint(int from, int to);
  Q_INVOKABLE void downloadMissingMaps();
  Q_INVOKABLE void close();
  // Follows the route, like START on Android; the start is moved to the position first if needed.
  Q_INVOKABLE void start();
  // Ends navigation and the route, like the Android Stop button.
  Q_INVOKABLE void stopNavigation();

  bool voiceAvailable() const;
  bool voiceEnabled() const;
  void setVoiceEnabled(bool enabled);
  QString voiceLanguage() const;
  void setVoiceLanguage(QString const & language);
  QString voiceLanguageName() const;
  int voiceLanguageIndex() const;
  QVariantList voiceLanguages() const;
  bool speechNoteInstalled() const;
  QString wantedVoiceLanguageName() const;
  bool wantedHasSpeechNoteVoice() const;
  bool announceStreets() const;
  void setAnnounceStreets(bool announce);
  // Looks for voices again, e.g. back from installing Speech Note or a voice.
  Q_INVOKABLE void refreshVoice();
  Q_INVOKABLE void openSpeechNote();
  // Speaks a turn notification, like "Test Voice Directions" on Android.
  Q_INVOKABLE void testVoice();

signals:
  void pointsChanged();
  void routerTypeChanged();
  void stateChanged();
  void optionsChanged();
  void navigationChanged();
  void voiceChanged();
  // Delivered queued: the result can arrive inside BuildRoute(), which OnRouteBuilt() may call again.
  void routeBuildingFinished(int code, QStringList absentCountries);

private:
  void AddPlacePoint(int type);
  void OnPointsChanged();
  void Build();
  // Forgets the built route, its summary and errors.
  void ClearResult();
  void OnRouteBuilt(int code, QStringList const & absentCountries);
  void SetError(QString const & title, QString const & message);
  void SetNavigationStyle(bool enabled);
  // Ends navigation and closes the route.
  void EndNavigation();
  void SetupVoice();
  std::string AppVoiceLanguage() const;
  std::string WantedVoiceLanguage() const;

  Framework & m_framework;
  bool m_building = false;
  bool m_built = false;
  QString m_summary;
  QString m_walkingDistance;
  QVariantList m_transitSteps;
  QString m_errorTitle;
  QString m_errorMessage;
  QStringList m_missingMaps;
  int m_storageSlot = 0;
  bool m_navigating = false;
  // START moved the start to the position; navigate once the route is rebuilt.
  bool m_startWhenBuilt = false;
  QVariantMap m_navigation;
  VoiceGuide * m_voice;
  // Speed camera warning.
  QMediaPlayer m_beep;
  int m_voiceTestIndex = 0;
};
}  // namespace sailfish
