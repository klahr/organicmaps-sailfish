#pragma once

#include "qt/qt_common/qtoglcontextfactory.hpp"

#include "drape_frontend/gui/skin.hpp"

#include "platform/location_service/location_service.hpp"

#include <QQuickItem>
#include <QTimer>

class QCompass;

#include <memory>
#include <optional>

class Framework;

namespace sailfish
{
class PlacePage;
class Routing;
}  // namespace sailfish

namespace sailfish
{
// Hosts the drape engine inside Qt Quick. Drape renders into offscreen framebuffers in
// contexts shared through QOpenGLContext::globalShareContext(); the item shows the last
// presented frame as a scene graph texture.
class MapItem
  : public QQuickItem
  , public location::LocationObserver
{
  Q_OBJECT
  // location::EMyPositionMode, drives the my position button.
  Q_PROPERTY(int myPositionMode READ myPositionMode NOTIFY myPositionModeChanged)
  // Bit mask of enabled Layer values.
  Q_PROPERTY(int enabledLayers READ enabledLayers NOTIFY layersChanged)
  Q_PROPERTY(bool darkStyle READ darkStyle NOTIFY layersChanged)
  Q_PROPERTY(sailfish::PlacePage * placePage READ placePage CONSTANT)
  Q_PROPERTY(sailfish::Routing * routing READ routing CONSTANT)
  // Height covered by panels at the bottom; routes and searches are fitted into the map above it.
  Q_PROPERTY(
      qreal viewportBottomInset READ viewportBottomInset WRITE setViewportBottomInset NOTIFY viewportBottomInsetChanged)
  Q_PROPERTY(bool trackRecording READ trackRecording NOTIFY trackRecordingChanged)
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
  // Text for "Share My Location", empty without a position.
  Q_INVOKABLE QString myPositionShareText() const;

  int myPositionMode() const { return m_myPositionMode; }
  PlacePage * placePage() const { return m_placePage.get(); }
  Routing * routing() const { return m_routing.get(); }
  qreal viewportBottomInset() const { return m_viewportBottomInset; }
  void setViewportBottomInset(qreal inset);
  bool trackRecording() const;
  qreal bottomWidgetsOffset() const { return m_bottomWidgetsOffset; }
  void setBottomWidgetsOffset(qreal offset);
  int enabledLayers() const;
  bool darkStyle() const;

signals:
  void myPositionModeChanged();
  void layersChanged();
  void bottomWidgetsOffsetChanged();
  void trackRecordingChanged();
  void viewportBottomInsetChanged();

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
  // The last fix is kept across runs and shown as obsolete until a fresh one arrives.
  void SaveLastLocation() const;
  void ShowLastLocation();

  // location::LocationObserver
  void OnLocationError(location::TLocationError errorCode) override;
  void OnLocationUpdated(location::GpsInfo const & info) override;

  Framework & m_framework;
  std::unique_ptr<qt::common::QtOGLContextFactory> m_contextFactory;
  std::unique_ptr<gui::Skin> m_skin;
  std::unique_ptr<location::LocationService> m_locationService;
  std::unique_ptr<PlacePage> m_placePage;
  std::unique_ptr<Routing> m_routing;
  // Points the my position arrow, like the rotation vector sensor on Android.
  QCompass * m_compass;
  location::EMyPositionMode m_myPositionMode = location::PendingPosition;
  std::optional<location::GpsInfo> m_lastLocation;
  bool m_lastLocationShown = false;
  // The first mode change arrives while the drape engine is still being created.
  bool m_engineCreated = false;
  QTimer m_updateTimer;
  double m_visualScale = 1.0;
  qreal m_bottomWidgetsOffset = 0;
  qreal m_viewportBottomInset = 0;
  bool m_inBackground = false;
};
}  // namespace sailfish
