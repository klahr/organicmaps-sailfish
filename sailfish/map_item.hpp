#pragma once

#include "qt/qt_common/qtoglcontextfactory.hpp"

#include "drape_frontend/gui/skin.hpp"

#include "platform/location_service/location_service.hpp"

#include <QQuickItem>
#include <QTimer>

#include <memory>

class Framework;

namespace sailfish
{
class PlacePage;
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

public:
  // Same set as the Android layers sheet.
  enum Layer
  {
    Outdoors,
    Isolines,
    Hiking,
    Cycling,
    Subway
  };
  Q_ENUM(Layer)

  explicit MapItem(QQuickItem * parent = nullptr);
  ~MapItem() override;

  Q_INVOKABLE void zoomIn();
  Q_INVOKABLE void zoomOut();
  Q_INVOKABLE void switchMyPositionMode();

  Q_INVOKABLE void setLayerEnabled(int layer, bool enabled);

  int myPositionMode() const { return m_myPositionMode; }
  PlacePage * placePage() const { return m_placePage.get(); }
  int enabledLayers() const;
  bool darkStyle() const;

signals:
  void myPositionModeChanged();
  void layersChanged();

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
  void SendMouseTouch(QMouseEvent * event, int touchType);
  void OnMyPositionModeChanged(location::EMyPositionMode mode);

  // location::LocationObserver
  void OnLocationError(location::TLocationError errorCode) override;
  void OnLocationUpdated(location::GpsInfo const & info) override;

  Framework & m_framework;
  std::unique_ptr<qt::common::QtOGLContextFactory> m_contextFactory;
  std::unique_ptr<gui::Skin> m_skin;
  std::unique_ptr<location::LocationService> m_locationService;
  std::unique_ptr<PlacePage> m_placePage;
  location::EMyPositionMode m_myPositionMode = location::PendingPosition;
  QTimer m_updateTimer;
  double m_visualScale = 1.0;
  bool m_inBackground = false;
};
}  // namespace sailfish
