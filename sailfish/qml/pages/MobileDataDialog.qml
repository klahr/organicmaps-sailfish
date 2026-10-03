import QtQuick 2.6
import Sailfish.Silica 1.0

// Confirms a map download over a cellular connection, like the Android mobile data dialog.
Dialog {
    property var acceptAction

    allowedOrientations: Orientation.All
    onAccepted: acceptAction()

    Column {
        width: parent.width

        DialogHeader {
            acceptText: appInfo.localized("download")
        }
        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            text: appInfo.localized("download_over_mobile_header")
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeLarge
            color: Theme.highlightColor
        }
        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            topPadding: Theme.paddingLarge
            text: appInfo.localized("download_over_mobile_message")
            wrapMode: Text.Wrap
            color: Theme.secondaryHighlightColor
        }
    }
}
