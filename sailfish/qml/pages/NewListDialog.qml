import QtQuick 2.6
import Sailfish.Silica 1.0

// Name prompt for a new bookmark list.
Dialog {
    // Called with the name, e.g. to move items to the new list.
    property var createAction

    canAccept: nameField.text.trim() !== ""
    onAccepted: createAction(nameField.text)

    Column {
        width: parent.width

        DialogHeader {}
        TextField {
            id: nameField
            width: parent.width
            focus: true
            label: appInfo.localized("bookmark_set_name")
            placeholderText: label
            EnterKey.enabled: text.trim() !== ""
            EnterKey.iconSource: "image://theme/icon-m-enter-accept"
            EnterKey.onClicked: accept()
        }
    }
}
