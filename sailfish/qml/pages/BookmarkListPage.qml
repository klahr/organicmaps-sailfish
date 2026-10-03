import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0
import "navigation.js" as Navigation

// Bookmarks and tracks of one list, like the Android bookmark list: its notes, a search field, sorting in
// sections, and the list settings and export in the pull down menu. Choosing an item shows it on the map.
Page {
    id: page

    property alias categoryId: bookmarks.categoryId
    // Shown until the model has the list.
    property string title

    allowedOrientations: Orientation.All

    BookmarksModel {
        id: bookmarks
    }

    // The menu text of a BookmarksModel sorting type, -1 for the default order.
    function sortingName(type) {
        switch (type) {
        case BookmarksModel.ByType: return appInfo.localized("sort_type")
        case BookmarksModel.ByDistance: return appInfo.localized("sort_distance")
        case BookmarksModel.ByTime: return appInfo.localized("sort_date")
        case BookmarksModel.ByName: return appInfo.localized("sort_name")
        default: return appInfo.localized("sort_default")
        }
    }

    SilicaListView {
        id: listView
        anchors.fill: parent
        model: bookmarks

        header: Column {
            width: listView.width

            PageHeader {
                title: bookmarks.name !== "" ? bookmarks.name : page.title
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: text !== ""
                text: bookmarks.description
                textFormat: Text.StyledText
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
                bottomPadding: Theme.paddingMedium
            }
            SearchField {
                width: parent.width
                placeholderText: appInfo.localized("search_in_the_list")
                onTextChanged: bookmarks.filter = text
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
            }
        }

        PullDownMenu {
            MenuItem {
                text: appInfo.localized("export_file_gpx")
                onClicked: bookmarksIO.exportCategory(bookmarks.categoryId, BookmarksIO.Gpx)
            }
            MenuItem {
                text: appInfo.localized("export_file")
                onClicked: bookmarksIO.exportCategory(bookmarks.categoryId, BookmarksIO.Kmz)
            }
            MenuItem {
                text: appInfo.localized("edit")
                onClicked: pageStack.push(Qt.resolvedUrl("EditListDialog.qml"), { bookmarks: bookmarks })
            }
            MenuItem {
                text: appInfo.localized("sort_bookmarks")
                onClicked: pageStack.push(sortPage)
            }
        }

        section {
            property: "block"
            delegate: SectionHeader {
                text: section
            }
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
                    visible: model.isTrack
                    text: appInfo.localized("export_file_gpx")
                    onClicked: bookmarksIO.exportTrack(model.itemId, BookmarksIO.Gpx)
                }
                MenuItem {
                    text: appInfo.localized("delete")
                    onClicked: item.remorseDelete(function() { bookmarks.remove(index) })
                }
            }
        }

        ViewPlaceholder {
            enabled: listView.count === 0 && bookmarks.filter === ""
            text: appInfo.localized("bookmarks_empty_list_title")
            hintText: appInfo.localized("bookmarks_empty_list_message")
        }

        VerticalScrollDecorator {}
    }

    // The sortings offered for the list, like the Android sorting sheet.
    Component {
        id: sortPage

        Page {
            allowedOrientations: Orientation.All

            SilicaListView {
                anchors.fill: parent
                header: PageHeader {
                    title: appInfo.localized("sort_bookmarks")
                }
                model: [-1].concat(bookmarks.sortingTypes)

                delegate: ListItem {
                    highlighted: down || modelData === bookmarks.sortingType
                    onClicked: {
                        bookmarks.sortingType = modelData
                        pageStack.pop()
                    }

                    Label {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * x
                        anchors.verticalCenter: parent.verticalCenter
                        text: page.sortingName(modelData)
                        highlighted: parent.highlighted
                    }
                }
            }
        }
    }
}
