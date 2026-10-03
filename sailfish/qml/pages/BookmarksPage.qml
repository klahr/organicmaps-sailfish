import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0

// Bookmark lists, like the Android "Bookmarks and Tracks" screen.
Page {
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
                title: appInfo.localized("bookmarks_and_tracks")
            }
            SectionHeader {
                text: appInfo.localized("bookmark_lists")
            }
        }

        PullDownMenu {
            MenuItem {
                text: appInfo.localized("bookmarks_create_new_group")
                onClicked: pageStack.push(Qt.resolvedUrl("NewListDialog.qml"), { categories: categories })
            }
        }

        delegate: ListItem {
            id: item

            contentHeight: Theme.itemSizeMedium
            onClicked: pageStack.push(Qt.resolvedUrl("BookmarkListPage.qml"),
                                      { categoryId: model.categoryId, title: model.name })

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
                    // Counts after labels, which need no plural forms.
                    text: {
                        var s = appInfo.localized("bookmarks") + ": " + model.bookmarksCount
                        if (model.tracksCount > 0)
                            s += " • " + appInfo.localized("tracks_title") + ": " + model.tracksCount
                        return s
                    }
                    font.pixelSize: Theme.fontSizeSmall
                    color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                }
            }

            menu: ContextMenu {
                MenuItem {
                    text: appInfo.localized("zoom_to_country")
                    onClicked: {
                        categories.setVisible(index, true)
                        categories.showOnMap(index)
                        pageStack.pop()
                    }
                }
                MenuItem {
                    text: model.isVisible ? appInfo.localized("hide") : appInfo.localized("show")
                    onClicked: categories.setVisible(index, !model.isVisible)
                }
                MenuItem {
                    // The core keeps at least one list.
                    visible: listView.count > 1
                    text: appInfo.localized("delete")
                    onClicked: item.remorseDelete(function() { categories.deleteCategory(index) })
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
