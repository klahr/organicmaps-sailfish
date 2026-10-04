import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Share 1.0
import Nemo.KeepAlive 1.2
import Nemo.Notifications 1.0
import app.organicmaps 1.0
import "pages"
import "pages/notifications.js" as Notifications
import "cover"

ApplicationWindow {
    id: appWindow

    // The map page, for the cover.
    property Item mapPage

    initialPage: Component {
        MapPage {
            id: mapPageItem
            mapUpdateCount: downloads.updateCount
            Component.onCompleted: appWindow.mapPage = mapPageItem
        }
    }
    cover: Component {
        AppCover {
            mapPage: appWindow.mapPage
            countries: downloads
            onActivateRequested: appWindow.activate()
        }
    }
    allowedOrientations: defaultAllowedOrientations

    CountriesModel {
        id: downloads
    }


    // Exported bookmark and track files go to the share sheet, like on Android.
    ShareAction {
        id: fileShare
    }
    Connections {
        target: bookmarksIO
        onExportReady: {
            fileShare.mimeType = mimeType
            fileShare.resources = [fileUrl]
            fileShare.trigger()
        }
        onExportFailed: Notices.show(message, Notice.Short, Notice.Center)
        onImportFinished: Notices.show(message, Notice.Short, Notice.Center)
        onBackupFinished: if (!success) Notices.show(appInfo.localized("backup_failed"), Notice.Short, Notice.Center)
    }

    // Files and links opened with the app, see UrlHandler.
    Connections {
        target: urlHandler
        onActivated: activate()
    }

    // Offers updates of outdated maps once the map is up, like the Android map update dialog.
    Timer {
        running: true
        interval: 3000
        onTriggered: {
            if (pageStack.depth === 1 && !pageStack.busy && downloads.shouldOfferUpdate()
                    && appSettings.downloadPermission() !== AppSettings.DownloadDenied)
                pageStack.push(Qt.resolvedUrl("pages/UpdateMapsDialog.qml"), { countries: downloads })
        }
    }

    // Keep the device from suspending while maps download, so a blanked screen doesn't stall them.
    KeepAlive {
        enabled: downloads.downloadInProgress
    }

    // Download progress with Cancel, like the Android downloader notification. Silent: no preview.
    Notification {
        id: downloadNotification
        appName: "Organic Maps"
        appIcon: "organicmaps"
        summary: appInfo.localized("downloader_downloading") + " " + downloads.downloadingName
        progress: downloads.downloadingProgress
        remoteActions: [Notifications.openApp(),
                        Notifications.action("cancel", appInfo.localized("cancel"), "cancelDownloads")]
    }
    // Progress comes often; the notification follows it every few seconds.
    Timer {
        running: downloads.downloadInProgress && downloads.downloadingName !== ""
        repeat: true
        triggeredOnStart: true
        interval: 3000
        onTriggered: downloadNotification.publish()
    }
    Connections {
        target: downloads
        onDownloadInProgressChanged: if (!downloads.downloadInProgress) downloadNotification.close()
        // Failures are shown in the app; a notification tells when it is in the background, as on Android.
        onDownloadFailed: {
            if (Qt.application.state === Qt.ApplicationActive)
                return
            failedNotification.body = appInfo.localized("download_country_failed", [name])
            failedNotification.previewBody = failedNotification.body
            failedNotification.publish()
        }
    }
    Notification {
        id: failedNotification
        appName: "Organic Maps"
        appIcon: "organicmaps"
        summary: "Organic Maps"
        previewSummary: summary
        remoteActions: [Notifications.openApp()]
    }
    Connections {
        target: urlHandler
        onCancelDownloadsRequested: downloads.cancelAll()
    }
    Component.onDestruction: downloadNotification.close()
}
