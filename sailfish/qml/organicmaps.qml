import QtQuick 2.6
import Sailfish.Silica 1.0
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

    // Keep the device from suspending while maps download, so a blanked screen doesn't stall them.
    KeepAlive {
        enabled: downloads.downloadInProgress
    }
}
