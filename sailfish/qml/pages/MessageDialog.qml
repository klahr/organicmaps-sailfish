import QtQuick 2.6
import Sailfish.Silica 1.0

// A question with a message, like the Android alert dialogs; acceptAction runs once the accepted dialog is
// gone, so that it can open another page.
Dialog {
    id: dialog

    property string title
    property string message
    property string acceptText
    property string cancelText
    property var acceptAction

    allowedOrientations: Orientation.All
    property bool runAction
    onAccepted: runAction = !!acceptAction
    onStatusChanged: {
        if (status === PageStatus.Inactive && runAction) {
            runAction = false
            acceptAction()
        }
    }

    Column {
        width: parent.width

        DialogHeader {
            acceptText: dialog.acceptText !== "" ? dialog.acceptText : defaultAcceptText
            cancelText: dialog.cancelText !== "" ? dialog.cancelText : defaultCancelText
        }
        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            visible: text !== ""
            text: title
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeLarge
            color: Theme.highlightColor
        }
        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            topPadding: Theme.paddingLarge
            text: message
            wrapMode: Text.Wrap
            color: Theme.secondaryHighlightColor
        }
    }
}
