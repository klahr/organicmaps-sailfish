import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0
import "colors.js" as Colors

Page {
    id: page

    property SearchModel search
    // A result was chosen and is shown in the place page.
    signal resultActivated()

    allowedOrientations: Orientation.All

    onStatusChanged: {
        if (status === PageStatus.Active && search.query === "")
            searchField.forceActiveFocus()
    }

    function showOnMap() {
        search.showOnMap()
        pageStack.pop()
    }

    SearchField {
        id: searchField
        width: parent.width
        // Clear the camera notch in portrait, like the Silica PageHeader does.
        y: page.orientation === Orientation.Portrait ? Screen.topCutout.height : 0
        placeholderText: appInfo.localized("search")
        text: search.query
        onTextChanged: search.query = text
        // Typing breaks the text binding, so follow queries set by categories, history and suggestions.
        Connections {
            target: search
            onQueryChanged: if (searchField.text !== search.query) searchField.text = search.query
        }
        inputMethodHints: Qt.ImhNoPredictiveText
        EnterKey.iconSource: "image://theme/icon-m-enter-accept"
        EnterKey.enabled: resultsView.count > 0
        EnterKey.onClicked: page.showOnMap()
    }

    // History and categories, the two Android tabs, as sections while nothing is typed.
    SilicaListView {
        id: categoriesView
        anchors {
            top: searchField.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        clip: true
        visible: search.query === ""
        model: search.categories()

        header: Column {
            width: categoriesView.width

            SectionHeader {
                text: appInfo.localized("history")
                visible: appSettings.searchHistory && search.history.length > 0
            }
            Repeater {
                model: search.history

                MenuRow {
                    icon: "image://theme/icon-m-history"
                    text: modelData
                    onClicked: search.query = modelData
                }
            }
            MenuRow {
                visible: appSettings.searchHistory && search.history.length > 0
                icon: "image://theme/icon-m-cancel"
                text: appInfo.localized("clear_search")
                onClicked: search.clearHistory()
            }
            SectionHeader {
                text: appInfo.localized("categories")
            }
        }

        delegate: MenuRow {
            icon: Qt.resolvedUrl("../../icons/categories/ic_" + modelData.key
                                 + (Theme.colorScheme === Theme.LightOnDark ? "_night" : "") + ".svg")
            text: modelData.name
            onClicked: search.searchCategory(modelData.name)
        }

        VerticalScrollDecorator {}
    }

    SilicaListView {
        id: resultsView
        anchors.fill: categoriesView
        clip: true
        visible: !categoriesView.visible
        model: search

        delegate: ListItem {
            contentHeight: column.height + 2 * Theme.paddingMedium

            // Android row: title, then details with the opening state, then address with the distance.
            Column {
                id: column
                x: Theme.horizontalPageMargin
                y: Theme.paddingMedium
                width: parent.width - 2 * x

                Label {
                    width: parent.width
                    text: model.name
                    textFormat: Text.StyledText
                    truncationMode: TruncationMode.Fade
                }
                ResultLine {
                    text: model.description
                    trailingText: model.openStatus
                    trailingColor: model.openState === SearchModel.Open ? Colors.open
                                 : model.openState === SearchModel.ClosingSoon ? Colors.closingSoon : Colors.closed
                }
                ResultLine {
                    text: model.address
                    textFormat: Text.StyledText
                    trailingText: model.distance
                    trailingColor: Theme.highlightColor
                }
            }

            onClicked: {
                if (search.activate(index)) {
                    page.resultActivated()
                    pageStack.pop()
                }
            }
        }

        ViewPlaceholder {
            enabled: resultsView.count === 0 && !search.searching
            text: appInfo.localized("search_not_found")
            hintText: appInfo.localized("search_not_found_query")
        }

        VerticalScrollDecorator {}
    }

    BusyIndicator {
        anchors.centerIn: resultsView
        size: BusyIndicatorSize.Large
        running: search.searching && resultsView.count === 0
    }
}
