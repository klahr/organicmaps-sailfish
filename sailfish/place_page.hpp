#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>

#include <string_view>
#include <vector>

class Framework;

namespace sailfish
{
// The selected map object, following the Framework place page events. Kept free of map headers,
// which Qt 5.6 moc can't parse.
class PlacePage : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool open READ open NOTIFY changed)
  Q_PROPERTY(QString title READ title NOTIFY changed)
  Q_PROPERTY(QString subtitle READ subtitle NOTIFY changed)
  Q_PROPERTY(QString address READ address NOTIFY changed)
  Q_PROPERTY(QString distance READ distance NOTIFY distanceChanged)
  // Direction to the place relative to the device heading in degrees, negative without a position.
  Q_PROPERTY(double azimuth READ azimuth NOTIFY distanceChanged)
  // Same text as the Android share button, and the geo: link of "Open in Another App".
  Q_PROPERTY(QString shareText READ shareText NOTIFY changed)
  Q_PROPERTY(QString geoUri READ geoUri NOTIFY changed)
  Q_PROPERTY(bool isBookmark READ isBookmark NOTIFY changed)
  Q_PROPERTY(bool isTrack READ isTrack NOTIFY changed)
  // The selected bookmark or track, for BookmarkEditor.
  Q_PROPERTY(quint64 userMarkId READ userMarkId NOTIFY changed)
  // The place can be edited in OpenStreetMap.
  Q_PROPERTY(bool canEdit READ canEdit NOTIFY changed)
  // "Add Place to OpenStreetMap" is offered, e.g. for an area or an empty spot.
  Q_PROPERTY(bool canAddPlace READ canAddPlace NOTIFY changed)
  // "Add business" is offered for a building, to add a business inside it.
  Q_PROPERTY(bool canAddBusiness READ canAddBusiness NOTIFY changed)
  // The map here is downloaded and editable; edit and add are disabled otherwise, as on Android.
  Q_PROPERTY(bool editable READ editable NOTIFY changed)
  Q_PROPERTY(QString coordinates READ coordinates NOTIFY changed)
  // Bare values of all coordinate formats available at the place, offered for copying like on Android.
  Q_PROPERTY(QStringList coordinateValues READ coordinateValues NOTIFY changed)
  // Opening hours like the Android preview: state line (OpenState) with a description, and the raw
  // schedule shown when expanded.
  Q_PROPERTY(int openState READ openState NOTIFY changed)
  Q_PROPERTY(QString openTitle READ openTitle NOTIFY changed)
  Q_PROPERTY(QString openDescription READ openDescription NOTIFY changed)
  Q_PROPERTY(QString openingHours READ openingHours NOTIFY changed)
  Q_PROPERTY(QString wikiDescription READ wikiDescription NOTIFY changed)
  Q_PROPERTY(QString wikiUrl READ wikiUrl NOTIFY changed)
  // A track's statistics as {label, value} rows, like the Android elevation profile header.
  Q_PROPERTY(QVariantList trackStats READ trackStats NOTIFY changed)
  // A track's elevation profile as [distance, altitude] pairs in meters, flattened; empty without altitudes.
  Q_PROPERTY(QVariantList elevationProfile READ elevationProfile NOTIFY changed)
  Q_PROPERTY(double trackLength READ trackLength NOTIFY changed)
  Q_PROPERTY(QString minElevation READ minElevation NOTIFY changed)
  Q_PROPERTY(QString maxElevation READ maxElevation NOTIFY changed)
  // Distances along the track in meters of the point chosen on the profile or the map, and of the position;
  // negative when there is none.
  Q_PROPERTY(double elevationActivePoint READ elevationActivePoint NOTIFY elevationPointsChanged)
  Q_PROPERTY(double elevationMyPosition READ elevationMyPosition NOTIFY elevationPointsChanged)
  // Public transport routes through a stop, like the Android route row: the refs, the one shown on the map in
  // bold, and the routes as {label, color} ("ref: from → to") to choose from. Empty for other places.
  Q_PROPERTY(QString routeRefs READ routeRefs NOTIFY changed)
  Q_PROPERTY(QVariantList routes READ routes NOTIFY changed)
  Q_PROPERTY(bool isTramStop READ isTramStop NOTIFY changed)
  // Detail rows as {icon, text, url}; url is opened externally when set.
  Q_PROPERTY(QVariantList details READ details NOTIFY changed)

public:
  enum OpenState
  {
    OpenUnknown,
    Open,
    Closed
  };
  Q_ENUM(OpenState)

  explicit PlacePage(Framework & framework, QObject * parent = nullptr);
  ~PlacePage() override;

  bool open() const { return m_open; }
  QString title() const { return m_title; }
  QString subtitle() const { return m_subtitle; }
  QString address() const { return m_address; }
  QString distance() const { return m_distance; }
  double azimuth() const { return m_azimuth; }
  QString shareText() const { return m_shareText; }
  QString geoUri() const { return m_geoUri; }
  bool isBookmark() const { return m_isBookmark; }
  bool isTrack() const { return m_isTrack; }
  quint64 userMarkId() const { return m_userMarkId; }
  bool canEdit() const { return m_canEdit; }
  bool canAddPlace() const { return m_canAddPlace; }
  bool canAddBusiness() const { return m_canAddBusiness; }
  bool editable() const { return m_editable; }
  QString coordinates() const { return m_coordinates; }
  QStringList coordinateValues() const { return m_coordinateValues; }
  QVariantList details() const { return m_details; }
  int openState() const { return m_openState; }
  QString openTitle() const { return m_openTitle; }
  QString openDescription() const { return m_openDescription; }
  QString openingHours() const { return m_openingHours; }
  QString wikiDescription() const { return m_wikiDescription; }
  QString wikiUrl() const { return m_wikiUrl; }
  QString routeRefs() const { return m_routeRefs; }
  QVariantList routes() const { return m_routes; }
  bool isTramStop() const { return m_isTramStop; }
  QVariantList trackStats() const { return m_trackStats; }
  QVariantList elevationProfile() const { return m_elevationProfile; }
  double trackLength() const { return m_trackLength; }
  QString minElevation() const { return m_minElevation; }
  QString maxElevation() const { return m_maxElevation; }
  double elevationActivePoint() const;
  double elevationMyPosition() const;

  Q_INVOKABLE void close();
  // Cycles the coordinates format and remembers it, like a tap on the mobile place pages.
  Q_INVOKABLE void nextCoordinatesFormat();
  // Saves the place to the last edited list, or deletes its bookmark, like the Android Save button.
  Q_INVOKABLE void toggleBookmark();
  // Marks the point at the distance along the selected track on the map, like a tap on the Android profile.
  Q_INVOKABLE void setElevationActivePoint(double distance);
  // Shows a route of routes on the map, like choosing it in the Android routes popup.
  Q_INVOKABLE void showRoute(int index);

  // Called on every location update; does nothing without a selected place.
  void UpdateDistance();
  // Compass heading in radians from true north.
  void SetNorth(double north);

signals:
  void changed();
  void distanceChanged();
  void elevationPointsChanged();

private:
  void Update();
  void UpdateOpeningHours(std::string_view openingHours);
  void UpdateTrack();
  void UpdateRouteRefs();

  Framework & m_framework;
  bool m_open = false;
  QString m_title;
  QString m_subtitle;
  QString m_address;
  QString m_distance;
  double m_azimuth = -1.0;
  double m_north = 0.0;
  QString m_shareText;
  QString m_geoUri;
  bool m_isBookmark = false;
  bool m_isTrack = false;
  quint64 m_userMarkId = 0;
  bool m_canEdit = false;
  bool m_canAddPlace = false;
  bool m_canAddBusiness = false;
  bool m_editable = false;
  QString m_coordinates;
  QStringList m_coordinateValues;
  QVariantList m_details;
  int m_openState = OpenUnknown;
  QString m_openTitle;
  QString m_openDescription;
  QString m_openingHours;
  QString m_wikiDescription;
  QString m_wikiUrl;
  QString m_routeRefs;
  QVariantList m_routes;
  std::vector<uint32_t> m_routeIds;
  bool m_isTramStop = false;
  QVariantList m_trackStats;
  QVariantList m_elevationProfile;
  double m_trackLength = 0;
  QString m_minElevation;
  QString m_maxElevation;
};
}  // namespace sailfish
