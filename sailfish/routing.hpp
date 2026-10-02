#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

class Framework;

namespace sailfish
{
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
  // The Android navigation panels: turnIcon, distanceToTurn, street, nextTurnIcon, timeLeft, distanceLeft,
  // arrival, speed, speedUnits, speedLimit and progress (0..1).
  Q_PROPERTY(QVariantMap navigation READ navigation NOTIFY navigationChanged)

public:
  enum RouterType
  {
    Vehicle,
    Pedestrian,
    Bicycle,
    Transit,
    Ruler
  };
  Q_ENUM(RouterType)

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
  Q_INVOKABLE void removePoint(int index);
  Q_INVOKABLE void movePoint(int from, int to);
  Q_INVOKABLE void downloadMissingMaps();
  Q_INVOKABLE void close();
  // Follows the route, like START on Android; the start is moved to the position first if needed.
  Q_INVOKABLE void start();
  // Ends navigation and the route, like the Android Stop button.
  Q_INVOKABLE void stopNavigation();

signals:
  void pointsChanged();
  void routerTypeChanged();
  void stateChanged();
  void optionsChanged();
  void navigationChanged();
  // Route building results may come from routing threads; this is delivered queued to the GUI thread.
  void routeBuildingFinished(int code, QStringList absentCountries);

private:
  void AddPlacePoint(int type);
  void OnPointsChanged();
  void Build();
  void OnRouteBuilt(int code, QStringList const & absentCountries);
  void SetError(QString const & title, QString const & message);
  void SetNavigationStyle(bool enabled);

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
};
}  // namespace sailfish
