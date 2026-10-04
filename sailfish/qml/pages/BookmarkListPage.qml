import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Share 1.0
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
        // Rows change meaning with the list.
        onModelReset: page.selected = []
    }

    // Selection mode, like on Android: rows to move, recolor or delete together.
    property bool selecting
    property var selected: []
    onSelectingChanged: selected = []

    function toggleSelected(row) {
        var selected = page.selected.slice()
        var i = selected.indexOf(row)
        if (i >= 0)
            selected.splice(i, 1)
        else
            selected.push(row)
        page.selected = selected
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
                title: page.selecting ? appInfo.localized("select") + " (" + page.selected.length + ")"
                     : bookmarks.name !== "" ? bookmarks.name : page.title
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
                visible: page.selecting
                text: appInfo.localized("cancel")
                onClicked: page.selecting = false
            }
            MenuItem {
                visible: page.selecting
                text: appInfo.localized(page.selected.length < listView.count ? "select_all" : "deselect_all")
                onClicked: {
                    var all = []
                    if (page.selected.length < listView.count)
                        for (var i = 0; i < listView.count; ++i)
                            all.push(i)
                    page.selected = all
                }
            }
            MenuItem {
                visible: !page.selecting && listView.count > 0
                text: appInfo.localized("select")
                onClicked: page.selecting = true
            }
            MenuItem {
                visible: !page.selecting
                text: appInfo.localized("export_file_gpx")
                onClicked: bookmarksIO.exportCategory(bookmarks.categoryId, BookmarksIO.Gpx)
            }
            MenuItem {
                visible: !page.selecting
                text: appInfo.localized("export_file")
                onClicked: bookmarksIO.exportCategory(bookmarks.categoryId, BookmarksIO.Kmz)
            }
            MenuItem {
                visible: !page.selecting
                text: appInfo.localized("edit")
                onClicked: pageStack.push(Qt.resolvedUrl("EditListDialog.qml"), { bookmarks: bookmarks })
            }
            MenuItem {
                visible: !page.selecting
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

            readonly property bool selected: page.selected.indexOf(index) >= 0

            contentHeight: Theme.itemSizeMedium
            highlighted: down || selected || menuOpen
            menu: page.selecting ? null : itemMenu
            onClicked: {
                if (page.selecting) {
                    page.toggleSelected(index)
                    return
                }
                bookmarks.showOnMap(index)
                Navigation.popToMap(pageStack)
            }

            // The item color, like the Android list icons; a check mark while selected.
            Rectangle {
                id: colorDot
                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                width: Theme.iconSizeSmall
                height: width
                radius: width / 2
                color: model.color || "transparent"

                Icon {
                    anchors.centerIn: parent
                    visible: item.selected
                    source: "image://theme/icon-s-accept"
                    color: "white"
                }
            }
            // Shows or hides a track on the map, like the Android eye button.
            IconButton {
                id: eyeButton
                anchors {
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin - Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }
                visible: model.isTrack && !page.selecting
                icon.source: Qt.resolvedUrl("../../icons/bookmarks/" + (model.isVisible ? "ic_show.svg" : "ic_hide.svg"))
                icon.sourceSize: Qt.size(Theme.iconSizeMedium, Theme.iconSizeMedium)
                onClicked: bookmarks.setTrackVisible(index, !model.isVisible)
            }

            Column {
                anchors {
                    left: colorDot.right
                    leftMargin: Theme.paddingLarge
                    right: eyeButton.visible ? eyeButton.left : parent.right
                    rightMargin: eyeButton.visible ? 0 : Theme.horizontalPageMargin
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
                    visible: text !== ""
                    // The distance first, like on Android.
                    text: model.distance && model.type ? model.distance + " • " + model.type
                                                       : model.distance || model.type || ""
                    font.pixelSize: Theme.fontSizeSmall
                    color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    truncationMode: TruncationMode.Fade
                }
            }

            Component {
                id: itemMenu

                ContextMenu {
                    MenuItem {
                        text: appInfo.localized(model.isTrack ? "edit_track" : "placepage_edit_bookmark_button")
                        onClicked: pageStack.push(Qt.resolvedUrl("EditBookmarkPage.qml"),
                                                  { itemId: model.itemId, isTrack: model.isTrack })
                    }
                    MenuItem {
                        visible: !model.isTrack
                        text: appInfo.localized("share")
                        onClicked: {
                            bookmarkShare.resources = [{ "type": "text/plain", "data": bookmarks.shareText(index),
                                                         "name": model.name }]
                            bookmarkShare.trigger()
                        }
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

    // Actions on the selected items.
    DockedPanel {
        id: selectionPanel
        width: parent.width
        height: selectionButtons.height + 2 * Theme.paddingLarge
        dock: Dock.Bottom
        open: page.selecting && page.selected.length > 0

        ButtonLayout {
            id: selectionButtons
            anchors.centerIn: parent
            width: parent.width

            Button {
                text: appInfo.localized("move")
                onClicked: pageStack.push(movePage)
            }
            Button {
                text: appInfo.localized("change_color")
                onClicked: pageStack.push(Qt.resolvedUrl("ColorPickerPage.qml"), {
                    title: appInfo.localized("change_color"),
                    colors: bookmarks.colors,
                    chosen: function(colorIndex) {
                        bookmarks.setRowsColor(page.selected, colorIndex)
                        page.selecting = false
                    }
                })
            }
            Button {
                text: appInfo.localized("delete")
                onClicked: {
                    var rows = page.selected
                    Remorse.popupAction(page, appInfo.localized("delete"), function() {
                        bookmarks.removeRows(rows)
                        page.selecting = false
                    })
                }
            }
        }
    }

    // The lists to move the selected items to.
    Component {
        id: movePage

        Page {
            allowedOrientations: Orientation.All

            SilicaListView {
                anchors.fill: parent
                header: PageHeader {
                    title: appInfo.localized("select_list")
                }
                model: bookmarks.categories.filter(function(category) { return category.id !== bookmarks.categoryId })

                delegate: ListItem {
                    onClicked: {
                        bookmarks.moveRows(page.selected, modelData.id)
                        page.selecting = false
                        pageStack.pop()
                    }

                    Label {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * x
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.name
                        truncationMode: TruncationMode.Fade
                        highlighted: parent.highlighted
                    }
                }
            }
        }
    }

    ShareAction {
        id: bookmarkShare
        mimeType: "text/plain"
    }
}
