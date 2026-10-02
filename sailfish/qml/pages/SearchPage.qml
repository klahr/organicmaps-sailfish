import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0

Page {
    id: page

    property SearchModel search

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
        placeholderText: qsTr("Search")
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
                text: qsTr("History")
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
                text: qsTr("Clear Search History")
                onClicked: search.clearHistory()
            }
            SectionHeader {
                text: qsTr("Categories")
            }
        }

        delegate: BackgroundItem {
            width: categoriesView.width
            height: Theme.itemSizeMedium

            Image {
                id: categoryIcon
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                width: Theme.iconSizeMedium
                height: width
                sourceSize: Qt.size(width, height)
                source: Qt.resolvedUrl("../../icons/categories/ic_" + modelData.key
                                       + (Theme.colorScheme === Theme.LightOnDark ? "_night" : "") + ".svg")
            }
            Label {
                anchors {
                    left: categoryIcon.right
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                text: modelData.name
                truncationMode: TruncationMode.Fade
                color: highlighted ? Theme.highlightColor : Theme.primaryColor
            }
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
                    trailingColor: model.openState === SearchModel.Open ? "#4caf50"
                                 : model.openState === SearchModel.ClosingSoon ? "#ffc107" : "#f44336"
                }
                ResultLine {
                    text: model.address
                    textFormat: Text.StyledText
                    trailingText: model.distance
                    trailingColor: Theme.highlightColor
                }
            }

            onClicked: {
                if (search.activate(index))
                    pageStack.pop()
            }
        }

        ViewPlaceholder {
            enabled: resultsView.count === 0 && !search.searching
            text: qsTr("Nothing found")
            hintText: qsTr("Check the spelling or download the maps of the area")
        }

        VerticalScrollDecorator {}
    }

    BusyIndicator {
        anchors.centerIn: resultsView
        size: BusyIndicatorSize.Large
        running: search.searching && resultsView.count === 0
    }
}
