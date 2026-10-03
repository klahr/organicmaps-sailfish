import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0

// The category of a new place, like the Android FeatureCategoryFragment: recently used first, then all,
// or the search results. The editor for the new place follows.
Page {
    id: page

    // The position chosen on the map.
    property real lat
    property real lon

    allowedOrientations: Orientation.All

    PlaceEditor {
        id: categories
    }

    // Recent and all categories under their headers, as on Android; search results without them.
    function load(query) {
        var items = categories.categories(query)
        if (items.length === 0 || !items[0].recent)
            return items
        var list = [{ header: appInfo.localized("editor_add_select_category_recent_subtitle") }]
        var i = 0
        for (; i < items.length && items[i].recent; ++i)
            list.push(items[i])
        list.push({ header: appInfo.localized("editor_add_select_category_all_subtitle") })
        return list.concat(items.slice(i))
    }

    SilicaListView {
        id: list
        anchors.fill: parent
        model: page.load("")
        currentIndex: -1

        header: Column {
            width: list.width

            PageHeader {
                title: appInfo.localized("editor_add_select_category")
            }
            SearchField {
                width: parent.width
                placeholderText: appInfo.localized("search")
                onTextChanged: list.model = page.load(text)
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
            }
        }

        delegate: BackgroundItem {
            readonly property bool isHeader: modelData.header !== undefined

            enabled: !isHeader
            height: isHeader ? sectionHeader.height : Theme.itemSizeSmall

            SectionHeader {
                id: sectionHeader
                visible: isHeader
                text: isHeader ? modelData.header : ""
            }
            Label {
                visible: !isHeader
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                text: isHeader ? "" : modelData.name
                truncationMode: TruncationMode.Fade
                color: highlighted ? Theme.highlightColor : Theme.primaryColor
            }
            onClicked: pageStack.replace(Qt.resolvedUrl("EditPlacePage.qml"),
                                         { newPlaceType: modelData.type, lat: page.lat, lon: page.lon })
        }

        ViewPlaceholder {
            enabled: list.count === 0
            text: appInfo.localized("search_not_found")
        }

        VerticalScrollDecorator {}
    }
}
