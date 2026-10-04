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
        id: editor
    }

    // Recent and all categories under their headers, as on Android; search results without them.
    function load(query) {
        var items = editor.categories(query)
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

        delegate: TextRow {
            readonly property bool isHeader: modelData.header !== undefined

            enabled: !isHeader
            height: isHeader ? sectionHeader.height : Theme.itemSizeSmall
            text: isHeader ? "" : modelData.name

            SectionHeader {
                id: sectionHeader
                visible: isHeader
                text: isHeader ? modelData.header : ""
            }
            onClicked: pageStack.replace(Qt.resolvedUrl("EditPlacePage.qml"),
                                         { newPlaceType: modelData.type, lat: page.lat, lon: page.lon })
        }

        // Places the editor can't add, and a note for the community instead, like the footer of the Android
        // category list.
        footer: Column {
            width: list.width
            spacing: Theme.paddingMedium
            topPadding: Theme.paddingLarge
            bottomPadding: Theme.paddingLarge

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: list.count === 0
                text: appInfo.localized("search_not_found")
                color: Theme.highlightColor
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: appInfo.localized("editor_category_unsuitable_text")
                textFormat: Text.StyledText
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
                linkColor: Theme.highlightColor
                onLinkActivated: Qt.openUrlExternally(link)
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: appInfo.localized("osm_note_hint")
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
            }
            EditsPublicNotice {}
            TextArea {
                id: noteArea
                width: parent.width
                placeholderText: appInfo.localized("editor_note_hint")
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                enabled: noteArea.text.trim() !== ""
                text: appInfo.localized("editor_report_problem_send_button")
                onClicked: {
                    appSettings.editsPublicNoticeShown = true
                    editor.createStandaloneNote(page.lat, page.lon, noteArea.text)
                    osmAccount.updateEdits()
                    osmAccount.uploadChanges()
                    Notices.show(appInfo.localized("osm_note_toast"), Notice.Short, Notice.Center)
                    pageStack.pop()
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
