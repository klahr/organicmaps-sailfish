import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0

// Bookmark lists, like the Android "Bookmarks and Tracks" screen.
Page {
    id: page

    allowedOrientations: Orientation.All

    BookmarkCategoriesModel {
        id: categories
    }

    SilicaListView {
        id: listView
        anchors.fill: parent
        model: categories

        header: Column {
            width: listView.width

            PageHeader {
                title: qsTr("Bookmarks and Tracks")
            }
            SectionHeader {
                text: qsTr("Lists")
            }
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("Create a new list")
                onClicked: pageStack.push(Qt.resolvedUrl("NewListDialog.qml"), { categories: categories })
            }
        }

        delegate: ListItem {
            id: item

            contentHeight: Theme.itemSizeMedium
            onClicked: pageStack.push(Qt.resolvedUrl("BookmarkListPage.qml"),
                                      { categoryId: model.categoryId, title: model.name })

            function remove() {
                remorseDelete(function() { categories.deleteCategory(index) })
            }

            // Shows or hides the list on the map, like the Android eye button.
            IconButton {
                id: visibilityButton
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin - Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }
                icon.source: Qt.resolvedUrl("../../icons/bookmarks/" + (model.isVisible ? "ic_show.svg" : "ic_hide.svg"))
                icon.sourceSize: Qt.size(Theme.iconSizeMedium, Theme.iconSizeMedium)
                onClicked: categories.setVisible(index, !model.isVisible)
            }
            Column {
                anchors {
                    left: visibilityButton.right
                    leftMargin: Theme.paddingMedium
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }

                Label {
                    width: parent.width
                    text: model.name
                    truncationMode: TruncationMode.Fade
                    highlighted: item.highlighted
                }
                Label {
                    width: parent.width
                    text: {
                        var s = model.bookmarksCount + " " + (model.bookmarksCount === 1 ? qsTr("bookmark") : qsTr("bookmarks"))
                        if (model.tracksCount > 0)
                            s += " • " + model.tracksCount + " " + (model.tracksCount === 1 ? qsTr("track") : qsTr("tracks"))
                        return s
                    }
                    font.pixelSize: Theme.fontSizeSmall
                    color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                }
            }

            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Show on map")
                    onClicked: {
                        categories.setVisible(index, true)
                        categories.showOnMap(index)
                        pageStack.pop()
                    }
                }
                MenuItem {
                    text: model.isVisible ? qsTr("Hide") : qsTr("Show")
                    onClicked: categories.setVisible(index, !model.isVisible)
                }
                MenuItem {
                    // The core keeps at least one list.
                    visible: listView.count > 1
                    text: qsTr("Delete")
                    onClicked: item.remove()
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
