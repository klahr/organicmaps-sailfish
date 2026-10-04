#pragma once

#include "qt/qt_common/qtoglcontextfactory.hpp"

#include "drape_frontend/gui/skin.hpp"

#include "platform/location_service/location_service.hpp"

#include <QQuickItem>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

#include <memory>

class Framework;
class QCompass;

namespace sailfish
{
class PlacePage;
class Routing;

// Hosts the drape engine inside Qt Quick. Drape renders into offscreen framebuffers in
// contexts shared through QOpenGLContext::globalShareContext(); the item shows the last
// presented frame as a scene graph texture.
class MapItem
  : public QQuickItem
  , public location::LocationObserver
{
  Q_OBJECT
  // MyPositionMode, drives the my position button.
  Q_PROPERTY(int myPositionMode READ myPositionMode NOTIFY myPositionModeChanged)
  // Bit mask of enabled Layer values.
  Q_PROPERTY(int enabledLayers READ enabledLayers NOTIFY layersChanged)
  Q_PROPERTY(sailfish::PlacePage * placePage READ placePage CONSTANT)
  Q_PROPERTY(sailfish::Routing * routing READ routing CONSTANT)
  // Height covered by panels at the bottom; routes and searches are fitted into the map above it.
  Q_PROPERTY(
      qreal viewportBottomInset READ viewportBottomInset WRITE setViewportBottomInset NOTIFY viewportBottomInsetChanged)
  Q_PROPERTY(bool trackRecording READ trackRecording NOTIFY trackRecordingChanged)
  // The recording so far, like the Android track recording place page: "1.2 km •
  // 15 min", and the elevation profile as distance and altitude pairs with its length and altitude range.
  Q_PROPERTY(QString recordingSummary READ recordingSummary NOTIFY recordingStatsChanged)
  Q_PROPERTY(QVariantList recordingProfile READ recordingProfile NOTIFY recordingStatsChanged)
  Q_PROPERTY(double recordingLength READ recordingLength NOTIFY recordingStatsChanged)
  Q_PROPERTY(QString recordingMinElevation READ recordingMinElevation NOTIFY recordingStatsChanged)
  Q_PROPERTY(QString recordingMaxElevation READ recordingMaxElevation NOTIFY recordingStatsChanged)
  // The last known position for the app cover as {address, coordinates, altitude, speed}; empty without one.
  Q_PROPERTY(QVariantMap positionInfo READ positionInfo NOTIFY positionInfoChanged)
  // Location is unavailable since the last fix, e.g. no GPS signal, like the Android recording notification.
  Q_PROPERTY(bool locationLost READ locationLost NOTIFY locationLostChanged)
  // The map shows the cross for "Add Place to OpenStreetMap", taps don't select places.
  Q_PROPERTY(bool choosingPosition READ choosingPosition NOTIFY choosingPositionChanged)
  // The region in the middle of the map while its map isn't downloaded, like the Android on-map downloader:
  // {countryId, name, size, status (a CountriesModel::Status), progress (0..1)}; empty otherwise.
  Q_PROPERTY(QVariantMap currentCountry READ currentCountry NOTIFY currentCountryChanged)
  // Height of the map buttons along the bottom edge; the scale line and attribution stay above them.
  Q_PROPERTY(
      qreal bottomWidgetsOffset READ bottomWidgetsOffset WRITE setBottomWidgetsOffset NOTIFY bottomWidgetsOffsetChanged)

public:
  // Same set as the Android layers sheet.
  enum Layer
  {
    Outdoors,
    Isolines,
    Hiking,
    Cycling,
    Subway,
    // Offered once a tile server is set in the settings, as on Android.
    Satellite
  };
  Q_ENUM(Layer)

  // Mirrors location::EMyPositionMode for QML.
  enum MyPositionMode
  {
    PendingPosition,
    NotFollowNoPosition,
    NotFollow,
    Follow,
    FollowAndRotate
  };
  Q_ENUM(MyPositionMode)

  explicit MapItem(QQuickItem * parent = nullptr);
  ~MapItem() override;

  Q_INVOKABLE void zoomIn();
  Q_INVOKABLE void zoomOut();
  Q_INVOKABLE void switchMyPositionMode();

  Q_INVOKABLE void setLayerEnabled(int layer, bool enabled);
  // Track recording as in the Android menu. Location keeps running in the background while recording.
  Q_INVOKABLE void startTrackRecording();
  Q_INVOKABLE bool isTrackRecordingEmpty() const;
  // Saves the recorded track, or discards it with an empty name.
  Q_INVOKABLE void stopTrackRecording(QString const & saveAsName);
  // Saves under the default name and stops, like "Stop and save" in the Android recording notification.
  Q_INVOKABLE void saveAndStopTrackRecording();
  // Text for "Share My Location", empty without a position.
  Q_INVOKABLE QString myPositionShareText() const;

  // Starts choosing the position of a new place, at the selected place when there is one. For a business the
  // cross stays inside the selected building, like "Add business" on Android.
  Q_INVOKABLE void startChoosingPosition(bool business = false);
  Q_INVOKABLE void stopChoosingPosition();
  // Ends choosing and returns [lat, lon] of the cross, or an empty list when no map is downloaded there.
  // [lat, lon] of the cross, empty outside downloaded maps when they are required (for a new place).
  Q_INVOKABLE QVariantList confirmChosenPosition(bool requireMaps = true);

  int myPositionMode() const { return m_myPositionMode; }
  PlacePage * placePage() const { return m_placePage.get(); }
  Routing * routing() const { return m_routing.get(); }
  qreal viewportBottomInset() const { return m_viewportBottomInset; }
  void setViewportBottomInset(qreal inset);
  QString recordingSummary() const { return m_recordingSummary; }
  QVariantMap positionInfo() const { return m_positionInfo; }
  bool locationLost() const { return m_locationLost; }
  // The map in the middle of the map is too old to edit, see MissingMapInfo(); empty otherwise. Like the iOS
  // "Update the Map to Contribute".
  Q_INVOKABLE QVariantMap mapToUpdateForEditing() const;
  // Downloads, retries or updates a map, see DownloadMap(), or cancels its download.
  Q_INVOKABLE void downloadMap(QString const & countryId);
  Q_INVOKABLE void cancelMap(QString const & countryId);
  // Hiking and cycling routes need newer maps here, like on iOS.
  Q_INVOKABLE bool needUpdateForRoutes() const;
  // Contour lines are on but not shown at this zoom, like the Android hint.
  Q_INVOKABLE bool isolinesNeedZoom() const;
  QVariantList recordingProfile() const { return m_recordingProfile; }
  double recordingLength() const { return m_recordingLength; }
  QString recordingMinElevation() const { return m_recordingMinElevation; }
  QString recordingMaxElevation() const { return m_recordingMaxElevation; }
  bool trackRecording() const;
  bool choosingPosition() const { return m_choosingPosition; }
  qreal bottomWidgetsOffset() const { return m_bottomWidgetsOffset; }
  void setBottomWidgetsOffset(qreal offset);
  int enabledLayers() const;
  QVariantMap currentCountry() const { return m_currentCountry; }

signals:
  void myPositionModeChanged();
  void layersChanged();
  void bottomWidgetsOffsetChanged();
  void trackRecordingChanged();
  void recordingStatsChanged();
  void positionInfoChanged();
  void locationLostChanged();
  // A short message for the user, like the Android toasts: location off, compass calibration, contour lines.
  void notice(QString const & message);
  // Contour lines need newer maps here, which Android offers to download.
  void isolinesNeedMaps();
  void choosingPositionChanged();
  void viewportBottomInsetChanged();
  void currentCountryChanged();

protected:
  QSGNode * updatePaintNode(QSGNode * oldNode, UpdatePaintNodeData *) override;
  void geometryChanged(QRectF const & newGeometry, QRectF const & oldGeometry) override;
  void touchEvent(QTouchEvent * event) override;
  void mousePressEvent(QMouseEvent * event) override;
  void mouseMoveEvent(QMouseEvent * event) override;
  void mouseReleaseEvent(QMouseEvent * event) override;

private:
  void OnWindowChanged(QQuickWindow * window);
  void OnApplicationStateChanged(Qt::ApplicationState state);
  void CreateEngine();
  void Resize(int width, int height);
  void UpdateWidgetLayout();
  void UpdateVisibleViewport();
  void SendMouseTouch(QMouseEvent * event, int touchType);
  void OnMyPositionModeChanged(location::EMyPositionMode mode);
  void OnCompassReading();
  // Downloads the map of the region shown when the user is in it, like auto-download on Android.
  void OnCurrentCountryChanged(std::string const & countryId);
  // Statistics follow a running recording, for its panel and the cover.
  void WatchRecording();
  void UpdatePositionInfo();
  void UpdateCurrentCountry();

  // location::LocationObserver
  void OnLocationError(location::TLocationError errorCode) override;
  void OnLocationUpdated(location::GpsInfo const & info) override;

  Framework & m_framework;
  std::unique_ptr<qt::common::QtOGLContextFactory> m_contextFactory;
  std::unique_ptr<gui::Skin> m_skin;
  std::unique_ptr<location::LocationService> m_locationService;
  std::unique_ptr<PlacePage> m_placePage;
  std::unique_ptr<Routing> m_routing;
  bool m_choosingPosition = false;
  // Points the my position arrow, like the rotation vector sensor on Android.
  QCompass * m_compass;
  location::EMyPositionMode m_myPositionMode = location::PendingPosition;
  QTimer m_updateTimer;
  double m_visualScale = 1.0;
  qreal m_bottomWidgetsOffset = 0;
  qreal m_viewportBottomInset = 0;
  bool m_inBackground = false;
  bool m_locationErrorShown = false;
  bool m_locationLost = false;
  QVariantMap m_positionInfo;
  // The last fix, for the cover, and when positionInfo was worked out.
  bool m_hasAltitude = false;
  double m_altitude = 0;
  double m_speed = -1;
  qint64 m_positionInfoMs = 0;
  bool m_calibrationShown = false;
  QString m_recordingSummary;
  QVariantList m_recordingProfile;
  double m_recordingLength = 0;
  QString m_recordingMinElevation;
  QString m_recordingMaxElevation;
  std::string m_currentCountryId;
  QVariantMap m_currentCountry;
  int m_storageSlot = 0;
};
}  // namespace sailfish
