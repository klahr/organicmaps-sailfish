import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0

// Offers to update the outdated maps once per new map data, like the Android map update dialog.
Dialog {
    // A CountriesModel.
    property QtObject countries

    allowedOrientations: Orientation.All
    Component.onCompleted: countries.setUpdateOffered()
    // Accepting also confirms a download over mobile data, which is explained here.
    onAccepted: countries.updateAll()

    Column {
        width: parent.width

        DialogHeader {
            acceptText: appInfo.localized("whats_new_auto_update_button_size", [countries.updateSize])
            cancelText: appInfo.localized("later")
        }
        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            text: appInfo.localized("whats_new_auto_update_title")
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeLarge
            color: Theme.highlightColor
        }
        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            topPadding: Theme.paddingLarge
            text: appInfo.localized("whats_new_auto_update_message")
            wrapMode: Text.Wrap
            color: Theme.secondaryHighlightColor
        }
        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            topPadding: Theme.paddingLarge
            visible: appSettings.downloadPermission() === AppSettings.DownloadAsk
            text: appInfo.localized("download_over_mobile_header") + " "
                  + appInfo.localized("download_over_mobile_message")
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.secondaryHighlightColor
        }
    }
}
