import QtQuick 2.6
import Sailfish.Silica 1.0

// Ends a track recording: accepting saves it under the given name, "Delete" discards it.
Dialog {
    id: dialog

    property QtObject map

    canAccept: nameField.text.trim() !== ""
    onAccepted: map.stopTrackRecording(nameField.text.trim())

    Column {
        width: parent.width

        DialogHeader {
            title: appInfo.localized("track_recording")
            acceptText: appInfo.localized("save")
            cancelText: appInfo.localized("cancel")
        }
        TextField {
            id: nameField
            width: parent.width
            label: qsTr("Track name")
            text: Qt.formatDateTime(new Date(), Locale.ShortFormat)
            EnterKey.enabled: text.trim() !== ""
            EnterKey.iconSource: "image://theme/icon-m-enter-accept"
            EnterKey.onClicked: dialog.accept()
        }
        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            text: appInfo.localized("delete")
            onClicked: {
                map.stopTrackRecording("")
                pageStack.pop()
            }
        }
    }
}
