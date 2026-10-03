import QtQuick 2.6
import Sailfish.Silica 1.0

// Name and notes of a bookmark list, like the Android list settings.
Dialog {
    // A BookmarksModel showing the list.
    property QtObject bookmarks

    allowedOrientations: Orientation.All
    canAccept: nameField.text.trim() !== ""
    onAccepted: bookmarks.setCategoryInfo(nameField.text, descriptionArea.text)

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            DialogHeader {
                title: appInfo.localized("edit")
            }
            TextField {
                id: nameField
                width: parent.width
                label: appInfo.localized("bookmark_set_name")
                placeholderText: label
                text: bookmarks.name
                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: descriptionArea.focus = true
            }
            TextArea {
                id: descriptionArea
                width: parent.width
                label: appInfo.localized("description")
                placeholderText: label
                text: bookmarks.description
            }
        }

        VerticalScrollDecorator {}
    }
}
