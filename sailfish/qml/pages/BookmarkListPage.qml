import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0

// Bookmarks of one list. Choosing one shows it on the map.
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
                pageStack.pop(pageStack.find(function(p) { return p.objectName === "mapPage" }))
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
                    text: qsTr("Delete")
                    onClicked: item.remorseDelete(function() { bookmarks.deleteBookmark(index) })
                }
            }
        }

        ViewPlaceholder {
            enabled: listView.count === 0
            text: qsTr("No bookmarks yet")
            hintText: qsTr("Tap a place on the map and save it")
        }

        VerticalScrollDecorator {}
    }
}
