#pragma once

#include <QMediaPlayer>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

class Framework;
struct RouteMarkData;

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
  // When the built route would arrive if started now, like on Android; empty for the ruler.
  Q_PROPERTY(QString arrival READ arrival NOTIFY stateChanged)
  // Transit routes: the walking part of the summary, and the legs as {icon, number, color} chips.
  Q_PROPERTY(QString walkingDistance READ walkingDistance NOTIFY stateChanged)
  Q_PROPERTY(QVariantList transitSteps READ transitSteps NOTIFY stateChanged)
  // Why the route could not be built, with the Android wording.
  Q_PROPERTY(QString errorTitle READ errorTitle NOTIFY stateChanged)
  Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)
  // Maps to download before the route can be built.
  Q_PROPERTY(QStringList missingMaps READ missingMaps NOTIFY stateChanged)
  Q_PROPERTY(QString missingMapsSize READ missingMapsSize NOTIFY stateChanged)
  // Elevation of built walking and bicycle routes, like the Android route chart: distance and altitude pairs in
  // one flat list, the route length, the altitude range and "↗ 120 m ↘ 80 m".
  Q_PROPERTY(QVariantList elevationProfile READ elevationProfile NOTIFY stateChanged)
  Q_PROPERTY(double elevationLength READ elevationLength NOTIFY stateChanged)
  Q_PROPERTY(QString minElevation READ minElevation NOTIFY stateChanged)
  Q_PROPERTY(QString maxElevation READ maxElevation NOTIFY stateChanged)
  Q_PROPERTY(QString ascentDescent READ ascentDescent NOTIFY stateChanged)
  // Distance of the point chosen on the chart and marked on the route, -1 without one.
  Q_PROPERTY(double elevationActivePoint READ elevationActivePoint NOTIFY elevationActivePointChanged)
  // Routing options: a RoutingOptions::Road mask of avoided roads, and stop reordering.
  Q_PROPERTY(int avoidRoads READ avoidRoads WRITE setAvoidRoads NOTIFY optionsChanged)
  Q_PROPERTY(bool routeOptimization READ routeOptimization WRITE setRouteOptimization NOTIFY optionsChanged)
  // START is offered for built car, walking and bicycle routes, as on Android.
  Q_PROPERTY(bool canStart READ canStart NOTIFY stateChanged)
  // Navigation starts from the position: the start isn't there, so START asks first, like on Android.
  Q_PROPERTY(bool startIsMyPosition READ startIsMyPosition NOTIFY pointsChanged)
  // The routing disclaimer was accepted once, before the first navigation, like on Android.
  Q_PROPERTY(bool disclaimerAccepted READ disclaimerAccepted NOTIFY disclaimerChanged)
  // Another stop fits, like the stop limit on Android.
  Q_PROPERTY(bool canAddStop READ canAddStop NOTIFY pointsChanged)
  // A route slot waits for a place, like waitForPoiPick on Android: a PointType, -1 for none, and the index in
  // points of the point to replace, -1 to add one.
  Q_PROPERTY(int pickType READ pickType NOTIFY pickChanged)
  Q_PROPERTY(int pickIndex READ pickIndex NOTIFY pickChanged)
  // The route failed while roads are avoided: the options are the likely cause, as on Android.
  Q_PROPERTY(bool optionsError READ optionsError NOTIFY stateChanged)
  // The built route was saved as a track since it was built, like the Android save button.
  Q_PROPERTY(bool routeSaved READ routeSaved NOTIFY stateChanged)
  Q_PROPERTY(bool navigating READ navigating NOTIFY navigationChanged)
  // The Android navigation panels: turnIcon, distanceToTurn, street, nextTurnIcon, hoursLeft, minutesLeft,
  // hourUnits, minuteUnits, arrival, distanceLeftValue, distanceLeftUnits, speed, speedLimit, speedCamLimitExceeded,
  // lanes as {icon, active} and progress (0..1).
  Q_PROPERTY(QVariantMap navigation READ navigation NOTIFY navigationChanged)
  // It is between sunset and sunrise at the position, for the scheduled appearance and the night style while
  // navigating; by the clock (7 to 18 is day) without a position, like on Android.
  Q_PROPERTY(bool darkOutside READ darkOutside NOTIFY darkOutsideChanged)
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
  // 0..100, like the Android voice volume.
  Q_PROPERTY(int voiceVolume READ voiceVolume WRITE setVoiceVolume NOTIFY voiceChanged)

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
  QString arrival() const { return m_arrival; }
  QString walkingDistance() const { return m_walkingDistance; }
  QVariantList transitSteps() const { return m_transitSteps; }
  QString errorTitle() const { return m_errorTitle; }
  QString errorMessage() const { return m_errorMessage; }
  QStringList missingMaps() const { return m_missingMaps; }
  QString missingMapsSize() const;
  QVariantList elevationProfile() const { return m_elevationProfile; }
  double elevationLength() const { return m_elevationLength; }
  QString minElevation() const { return m_minElevation; }
  QString maxElevation() const { return m_maxElevation; }
  QString ascentDescent() const { return m_ascentDescent; }
  double elevationActivePoint() const { return m_elevationActivePoint; }
  int avoidRoads() const;
  void setAvoidRoads(int roads);
  bool routeOptimization() const;
  void setRouteOptimization(bool enabled);
  bool canStart() const;
  bool startIsMyPosition() const;
  bool disclaimerAccepted() const;
  bool canAddStop() const;
  bool optionsError() const { return m_optionsError; }
  int pickType() const { return m_pickType; }
  int pickIndex() const { return m_pickIndex; }
  bool routeSaved() const { return m_routeSaved; }
  bool navigating() const { return m_navigating; }
  QVariantMap navigation() const { return m_navigation; }
  bool darkOutside() const { return m_darkOutside; }

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
  // The route point or road warning shown in the place page: removes it, or avoids that kind of road.
  Q_INVOKABLE void removePlacePoint();
  Q_INVOKABLE void avoidRoad(int road);
  Q_INVOKABLE void movePoint(int from, int to);
  Q_INVOKABLE void downloadMissingMaps();
  Q_INVOKABLE void acceptDisclaimer();
  Q_INVOKABLE void startPick(int type, int index);
  Q_INVOKABLE void cancelPick();
  // Fills the picked slot with the place shown in the place page, a position chosen on the map, or the position.
  Q_INVOKABLE void pickPlace();
  Q_INVOKABLE void pickPosition(double lat, double lon);
  Q_INVOKABLE void pickMyPosition();
  // Brings back the route of the last session, which is saved when the app is put aside, like on Android.
  Q_INVOKABLE void restoreSavedRoute();
  // Saves the built route as a track in the bookmarks and shows it.
  Q_INVOKABLE void saveRoute();
  Q_INVOKABLE void setElevationActivePoint(double distance);
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
  int voiceVolume() const;
  void setVoiceVolume(int volume);
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
  void disclaimerChanged();
  void pickChanged();
  // A short message to show, e.g. that no more stops fit.
  void message(QString const & text);
  void elevationActivePointChanged();
  void darkOutsideChanged();
  // Delivered queued: the result can arrive inside BuildRoute(), which OnRouteBuilt() may call again.
  void routeBuildingFinished(int code, QStringList absentCountries);

private:
  void AddPlacePoint(int type);
  // Adds or replaces a route point; a finish without a start starts from the position, like on Android.
  void AddPoint(RouteMarkData && point);
  // Puts the point into the picked slot.
  void PickPoint(RouteMarkData && point);
  void OnPointsChanged();
  void Build();
  // Forgets the built route, its summary and errors.
  void ClearResult();
  void OnRouteBuilt(int code, QStringList const & absentCountries);
  void SetError(QString const & title, QString const & message);
  // Keeps the route for the next start while it is planned or followed, like RoutingController.saveRoute().
  void SaveRouteForRestart();
  void LoadElevation();
  void ClearElevationActivePoint();
  void SetNavigationStyle(bool enabled);
  void UpdateDarkOutside();
  // Ends navigation and closes the route.
  void EndNavigation();
  void SetupVoice();
  std::string AppVoiceLanguage() const;
  std::string WantedVoiceLanguage() const;

  Framework & m_framework;
  bool m_building = false;
  bool m_built = false;
  bool m_routeSaved = false;
  bool m_optionsError = false;
  int m_pickType = -1;
  int m_pickIndex = -1;
  QString m_summary;
  QString m_arrival;
  QString m_walkingDistance;
  QVariantList m_transitSteps;
  QString m_errorTitle;
  QString m_errorMessage;
  QStringList m_missingMaps;
  QVariantList m_elevationProfile;
  double m_elevationLength = 0;
  QString m_minElevation;
  QString m_maxElevation;
  QString m_ascentDescent;
  double m_elevationActivePoint = -1;
  int m_storageSlot = 0;
  bool m_navigating = false;
  // START moved the start to the position; navigate once the route is rebuilt.
  bool m_startWhenBuilt = false;
  QVariantMap m_navigation;
  bool m_darkOutside = false;
  // When darkOutside was last worked out; sunset needs no check on every location update.
  qint64 m_darkOutsideCheckMs = 0;
  QTimer m_darkOutsideTimer;
  VoiceGuide * m_voice;
  // Speed camera warning.
  QMediaPlayer m_beep;
  int m_voiceTestIndex = 0;
};
}  // namespace sailfish
