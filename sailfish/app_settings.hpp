#pragma once

#include "indexer/map_style.hpp"

#include <QObject>
#include <QString>
#include <QVariantList>

class Framework;

namespace sailfish
{
// The default or outdoors map style in its dark or light variant, without the vehicle style of navigation.
MapStyle BaseMapStyle(bool dark, bool outdoors);

// The Android settings that apply to this port, available to QML as the appSettings context property.
// Core settings go through the Framework; UI only ones are kept in the same settings file.
class AppSettings : public QObject
{
  Q_OBJECT
  // MapAppearance; Auto follows the Sailfish ambience.
  Q_PROPERTY(int mapAppearance READ mapAppearance WRITE setMapAppearance NOTIFY changed)
  // measurement_utils::Units: 0 metric, 1 imperial.
  Q_PROPERTY(int units READ units WRITE setUnits NOTIFY changed)
  Q_PROPERTY(bool zoomButtons READ zoomButtons WRITE setZoomButtons NOTIFY changed)
  Q_PROPERTY(bool showDownloadedRegions READ showDownloadedRegions WRITE setShowDownloadedRegions NOTIFY changed)
  Q_PROPERTY(bool largeFonts READ largeFonts WRITE setLargeFonts NOTIFY changed)
  Q_PROPERTY(bool transliteration READ transliteration WRITE setTransliteration NOTIFY changed)
  Q_PROPERTY(bool keepScreenOn READ keepScreenOn WRITE setKeepScreenOn NOTIFY changed)
  Q_PROPERTY(bool searchHistory READ searchHistory WRITE setSearchHistory NOTIFY changed)
  Q_PROPERTY(QString mapLanguage READ mapLanguage WRITE setMapLanguage NOTIFY changed)
  Q_PROPERTY(QString mapLanguageName READ mapLanguageName NOTIFY changed)
  // Map languages as {code, name}.
  Q_PROPERTY(QVariantList mapLanguages READ mapLanguages CONSTANT)
  Q_PROPERTY(QString donateUrl READ donateUrl CONSTANT)
  Q_PROPERTY(bool buildings3d READ buildings3d WRITE setBuildings3d NOTIFY changed)
  Q_PROPERTY(bool autoDownload READ autoDownload WRITE setAutoDownload NOTIFY changed)
  // MobileData: map downloads over a cellular connection, like the Android Mobile Internet setting.
  Q_PROPERTY(int mobileData READ mobileData WRITE setMobileData NOTIFY changed)
  // Navigation group: the tilted map and zooming by speed while navigating.
  Q_PROPERTY(bool perspectiveView READ perspectiveView WRITE setPerspectiveView NOTIFY changed)
  Q_PROPERTY(bool autoZoom READ autoZoom WRITE setAutoZoom NOTIFY changed)
  // A dark map while navigating between sunset and sunrise, see Routing::darkOutside.
  Q_PROPERTY(bool autoNightInNavigation READ autoNightInNavigation WRITE setAutoNightInNavigation NOTIFY changed)
  // Debug logs in a file, which "Report a bug" shares, at logUrl.
  Q_PROPERTY(bool logging READ logging WRITE setLogging NOTIFY changed)
  Q_PROPERTY(QString logUrl READ logUrl CONSTANT)
  // The notice that edits go public was seen with a first edit or note, like Android's one-time dialog.
  Q_PROPERTY(bool editsPublicNoticeShown READ editsPublicNoticeShown WRITE setEditsPublicNoticeShown NOTIFY changed)
  // power_management::Scheme: Normal (never), EconomyMaximum (always) or Auto (low battery).
  Q_PROPERTY(int powerScheme READ powerScheme WRITE setPowerScheme NOTIFY changed)
  // routing::SpeedCameraManagerMode: Auto, Always or Never.
  Q_PROPERTY(int speedCamerasMode READ speedCamerasMode WRITE setSpeedCamerasMode NOTIFY changed)
  // settings::Placement: None, Right or Bottom.
  Q_PROPERTY(int bookmarksTextPlacement READ bookmarksTextPlacement WRITE setBookmarksTextPlacement NOTIFY changed)
  // Satellite imagery from a user provided XYZ tile server.
  Q_PROPERTY(bool bgTilesEnabled READ bgTilesEnabled NOTIFY changed)
  Q_PROPERTY(QString bgTilesUrl READ bgTilesUrl NOTIFY changed)
  Q_PROPERTY(int bgTilesCacheSize READ bgTilesCacheSize NOTIFY changed)
  Q_PROPERTY(int bgTilesOpacity READ bgTilesOpacity NOTIFY changed)

public:
  enum MapAppearance
  {
    AppearanceAuto,
    AppearanceLight,
    AppearanceDark,
    // Light from dawn till dusk, like on Android.
    AppearanceScheduled
  };
  Q_ENUM(MapAppearance)

  enum MobileData
  {
    MobileDataAsk,
    MobileDataAlways,
    MobileDataNever
  };
  Q_ENUM(MobileData)

  // Whether a map download may start now, see downloadPermission().
  enum DownloadPermission
  {
    DownloadAllowed,
    DownloadAsk,
    DownloadDenied
  };
  Q_ENUM(DownloadPermission)

  explicit AppSettings(Framework & framework, QObject * parent = nullptr);

  int mapAppearance() const;
  void setMapAppearance(int appearance);
  int units() const;
  void setUnits(int units);
  bool zoomButtons() const;
  void setZoomButtons(bool enabled);
  bool showDownloadedRegions() const;
  void setShowDownloadedRegions(bool enabled);
  bool largeFonts() const;
  void setLargeFonts(bool enabled);
  bool transliteration() const;
  void setTransliteration(bool enabled);
  bool keepScreenOn() const;
  void setKeepScreenOn(bool enabled);
  bool searchHistory() const;
  void setSearchHistory(bool enabled);
  QString mapLanguage() const;
  void setMapLanguage(QString const & code);
  QString mapLanguageName() const;
  QVariantList mapLanguages() const;
  QString donateUrl() const;
  bool buildings3d() const;
  void setBuildings3d(bool enabled);
  bool autoDownload() const { return IsAutoDownloadEnabled(); }
  bool perspectiveView() const;
  void setPerspectiveView(bool enabled);
  bool autoZoom() const;
  void setAutoZoom(bool enabled);
  bool logging() const;
  bool editsPublicNoticeShown() const;
  void setEditsPublicNoticeShown(bool shown);
  void setLogging(bool enabled);
  QString logUrl() const;
  // Bytes in the log file, shown with the setting like on iOS.
  Q_INVOKABLE qint64 logSize() const;
  // Starts the file log when the setting is on; called once at start.
  static void InitLogging();
  bool autoNightInNavigation() const;
  void setAutoNightInNavigation(bool enabled);
  void setAutoDownload(bool enabled);
  int mobileData() const;
  void setMobileData(int mobileData);
  // Downloads are allowed on other connections; on a cellular one the setting decides.
  Q_INVOKABLE int downloadPermission() const;
  int powerScheme() const;
  void setPowerScheme(int scheme);
  int speedCamerasMode() const;
  void setSpeedCamerasMode(int mode);
  int bookmarksTextPlacement() const;
  void setBookmarksTextPlacement(int placement);
  bool bgTilesEnabled() const;
  QString bgTilesUrl() const;
  int bgTilesCacheSize() const;
  int bgTilesOpacity() const;

  // Same as the Android Satellite Imagery screen.
  Q_INVOKABLE void setBackgroundTiles(bool enabled, QString const & url, int cacheSizeMb, int opacityPct);
  Q_INVOKABLE bool isWellFormedTilesUrl(QString const & url) const;

  // Switches the map style to its dark or light variant.
  Q_INVOKABLE void applyMapAppearance(bool dark);

  // UI only setting read by other C++ code.
  static bool IsSearchHistoryEnabled();
  static bool IsAutoDownloadEnabled();
  static bool IsOnMobileData();

signals:
  void changed();

private:
  Framework & m_framework;
};
}  // namespace sailfish
