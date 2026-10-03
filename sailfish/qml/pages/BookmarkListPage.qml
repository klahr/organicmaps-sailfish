import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0
import "navigation.js" as Navigation

// Bookmarks and tracks of one list. Choosing one shows it on the map.
Page {
    id: page

    property alias categoryId: bookmarks.categoryId
    property string title

    allowedOrientations: Orientation.All

    BookmarksModel {
        id: bookmarks
    }

    SilicaListView {
        id: listView
        anchors.fill: parent
        model: bookmarks

        header: PageHeader {
            title: page.title
        }

        delegate: ListItem {
            id: item

            contentHeight: Theme.itemSizeMedium
            onClicked: {
                bookmarks.showOnMap(index)
                Navigation.popToMap(pageStack)
            }

            Column {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                anchors.verticalCenter: parent.verticalCenter

                Label {
                    width: parent.width
                    text: model.name
                    truncationMode: TruncationMode.Fade
                    highlighted: item.highlighted
                }
                Label {
                    width: parent.width
                    visible: text !== ""
                    text: model.type
                    font.pixelSize: Theme.fontSizeSmall
                    color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    truncationMode: TruncationMode.Fade
                }
            }

            menu: ContextMenu {
                MenuItem {
                    text: appInfo.localized(model.isTrack ? "edit_track" : "placepage_edit_bookmark_button")
                    onClicked: pageStack.push(Qt.resolvedUrl("EditBookmarkPage.qml"),
                                              { itemId: model.itemId, isTrack: model.isTrack })
                }
                MenuItem {
                    text: appInfo.localized("delete")
                    onClicked: item.remorseDelete(function() { bookmarks.remove(index) })
                }
            }
        }

        ViewPlaceholder {
            enabled: listView.count === 0
            text: appInfo.localized("bookmarks_empty_list_title")
            hintText: appInfo.localized("bookmarks_empty_list_message")
        }

        VerticalScrollDecorator {}
    }
}
