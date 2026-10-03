import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Share 1.0
import Nemo.KeepAlive 1.2
import app.organicmaps 1.0
import "pages"

ApplicationWindow {
    initialPage: Component { MapPage { } }
    cover: undefined
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
}
