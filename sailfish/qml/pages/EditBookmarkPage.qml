import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0

// Edits a bookmark or a track like the Android EditBookmarkFragment: name, notes, color and list.
Dialog {
    id: dialog

    property var itemId
    property bool isTrack
    property int colorIndex: editor.colorIndex

    allowedOrientations: Orientation.All
    // A bookmark without a name shows its feature type; a track needs one.
    canAccept: !isTrack || nameField.text.trim() !== ""
    onAccepted: editor.save(nameField.text, notesArea.text, colorIndex, listBox.currentIndex)

    BookmarkEditor {
        id: editor
    }

    Component.onCompleted: {
        if (isTrack)
            editor.loadTrack(itemId)
        else
            editor.loadBookmark(itemId)
    }

    RemorsePopup {
        id: remorse
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        PullDownMenu {
            MenuItem {
                text: appInfo.localized("delete")
                onClicked: remorse.execute(appInfo.localized("delete"), function() {
                    editor.remove()
                    pageStack.pop()
                })
            }
        }

        Column {
            id: column
            width: parent.width

            DialogHeader {
                title: appInfo.localized(isTrack ? "edit_track" : "placepage_edit_bookmark_button")
                acceptText: appInfo.localized("save")
                cancelText: appInfo.localized("cancel")
            }
            TextField {
                id: nameField
                width: parent.width
                label: appInfo.localized(isTrack ? "placepage_track_name_hint" : "placepage_bookmark_name_hint")
                placeholderText: label
                text: editor.name
                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: notesArea.focus = true
            }
            TextArea {
                id: notesArea
                width: parent.width
                label: appInfo.localized("description")
                placeholderText: appInfo.localized("placepage_personal_notes_hint")
                text: editor.description
            }
            ComboBox {
                id: listBox
                width: parent.width
                label: appInfo.localized("list")
                currentIndex: editor.categoryIndex
                menu: ContextMenu {
                    Repeater {
                        model: editor.categories
                        MenuItem {
                            text: modelData
                        }
                    }
                }
            }
            SectionHeader {
                text: appInfo.localized("choose_color")
            }
            Grid {
                id: colorGrid

                readonly property real size: Theme.itemSizeExtraSmall

                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                spacing: Theme.paddingMedium
                columns: Math.max(1, Math.floor((width + spacing) / (size + spacing)))

                Repeater {
                    model: editor.colors
                    // A custom color (colorIndex -1) has no swatch here and stays until another one is chosen.
                    Rectangle {
                        width: colorGrid.size
                        height: width
                        radius: width / 2
                        color: modelData
                        border.width: index === dialog.colorIndex ? Theme.paddingSmall : 0
                        border.color: Theme.highlightColor

                        MouseArea {
                            anchors.fill: parent
                            onClicked: dialog.colorIndex = index
                        }
                    }
                }
            }
            Item {
                width: parent.width
                height: Theme.paddingLarge
            }
        }

        VerticalScrollDecorator {}
    }
}
