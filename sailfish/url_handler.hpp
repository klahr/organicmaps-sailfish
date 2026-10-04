#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>

class Framework;

namespace sailfish
{
class BookmarksIO;

// Opens what is given to the app, like the Android intent processors: bookmark and track files, and geo:, om://,
// ge0:// and https://omaps.app links. They come as arguments or over D-Bus from the Sailfish launcher, see
// organicmaps.desktop. Available to QML as urlHandler, which shows routes and searches.
class UrlHandler : public QObject
{
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "app.organicmaps.organicmaps")

public:
  UrlHandler(Framework & framework, BookmarksIO & bookmarksIO, QObject * parent = nullptr);

  // Registers the D-Bus service for the launcher; false when another instance has it.
  bool RegisterOnDBus();

public slots:
  // The X-Maemo-Method of organicmaps.desktop.
  Q_SCRIPTABLE void openUrl(QStringList const & urls);
  // The actions of the download and track recording notifications.
  Q_SCRIPTABLE void cancelDownloads() { emit cancelDownloadsRequested(); }
  Q_SCRIPTABLE void stopTrackRecording() { emit stopTrackRecordingRequested(); }

signals:
  // The window should come to the front.
  void activated();
  // Points as {lat, lon, name}, the first is the start and the last the finish.
  void routeRequested(int routerType, QVariantList const & points);
  // onMap: show the results on the map rather than the search page, like isSearchOnMap on Android.
  void searchRequested(QString const & query, bool onMap);
  // An om://crosshair link: pick a position for the app named, which may give a link back.
  void crosshairRequested(QString const & appName, QString const & backUrl);
  // The browser came back from the OpenStreetMap login with this code.
  void oauth2CodeReceived(QString const & code);
  void cancelDownloadsRequested();
  void stopTrackRecordingRequested();

private:
  void ProcessPending();
  void Process(QString const & url);

  Framework & m_framework;
  BookmarksIO & m_bookmarksIO;
  // Links wait for the map, which their requests move.
  QStringList m_pending;
  QTimer m_retryTimer;
};
}  // namespace sailfish
