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
            // Applied right away, like the Android list settings.
            Repeater {
                model: [{ tracks: false, text: appInfo.localized("change_all_bookmarks_color"),
                          done: appInfo.localized("toast_bookmarks_color_changed") },
                        { tracks: true, text: appInfo.localized("change_all_tracks_color"),
                          done: appInfo.localized("toast_tracks_color_changed") }]

                BackgroundItem {
                    width: column.width
                    onClicked: {
                        var option = modelData
                        pageStack.push(Qt.resolvedUrl("ColorPickerPage.qml"), {
                            title: option.text,
                                    chosen: function(colorIndex) {
                                bookmarks.setAllColor(option.tracks, colorIndex)
                                Notices.show(option.done, Notice.Short, Notice.Center)
                            }
                        })
                    }

                    Label {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * x
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.text
                        truncationMode: TruncationMode.Fade
                        color: parent.highlighted ? Theme.highlightColor : Theme.primaryColor
                    }
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
