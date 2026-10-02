import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0

Page {
    id: page
    objectName: "mapPage"
    allowedOrientations: Orientation.All
    backNavigation: false

    MapItem {
        id: map
        anchors.fill: parent
        // Keeps the scale line and attribution above the bottom row, like on Android.
        bottomWidgetsOffset: bottomButtons.height + bottomButtons.anchors.bottomMargin
    }

    MapButton {
        id: layersButton
        anchors {
            top: parent.top
            left: parent.left
            margins: Theme.dp(8)
        }
        source: Qt.resolvedUrl("../../icons/layers/ic_layers.svg")
        // Blue while a layer is applied, like on Android, and while choosing layers.
        highlighted: map.enabledLayers !== 0 || layersPanel.open
        onClicked: layersPanel.open = true
    }

    // Positions follow the Android map_buttons_layout_regular.xml (and layout-land); the look stays Silica.
    Column {
        id: rightButtons
        anchors {
            right: parent.right
            rightMargin: Theme.dp(8)
            // Portrait: ~104dp above the bottom row, measured on Android. Landscape: my position is level
            // with the row in the bottom right corner.
            bottom: page.isPortrait ? bottomButtons.top : parent.bottom
            bottomMargin: page.isPortrait ? Theme.dp(104) : bottomButtons.anchors.bottomMargin
        }
        spacing: Theme.dp(8)
        // Stay above the place page, as on Android.
        transform: Translate {
            y: -Math.max(0, placePagePanel.visibleSize - (page.height - rightButtons.y - rightButtons.height)
                            + Theme.dp(8))
        }

        MapButton {
            source: "image://theme/icon-m-add"
            onClicked: map.zoomIn()
        }
        MapButton {
            source: "image://theme/icon-m-remove"
            onClicked: map.zoomOut()
        }
        Item {
            width: 1
            height: Theme.dp(64)
        }
        // Mirrors location::EMyPositionMode, with the Android icons. The core cycles the modes: a tap centers
        // north up, further taps toggle rotating with the heading, panning the map stops following.
        MapButton {
            readonly property int pendingPosition: 0
            readonly property int notFollowNoPosition: 1
            readonly property int notFollow: 2
            readonly property int follow: 3
            readonly property int followAndRotate: 4
            readonly property var icons: ["", "ic_location_off", "ic_not_follow", "ic_follow", "ic_follow_and_rotate"]

            // While searching only the spinner shows, like the rotating ring on Android.
            source: map.myPositionMode === pendingPosition
                    ? "" : Qt.resolvedUrl("../../icons/myposition/" + icons[map.myPositionMode] + ".svg")
            highlighted: map.myPositionMode === follow || map.myPositionMode === followAndRotate
            busy: map.myPositionMode === pendingPosition
            onClicked: map.switchMyPositionMode()
        }
    }

    // Bottom row like on Android: help, search, bookmarks, menu. Centered in portrait, left aligned in
    // landscape; the gaps are about 0.6 of a button, as measured on the Android app.
    Row {
        id: bottomButtons
        // Positioned by x: switching between left and horizontalCenter anchors on rotation can leave both set.
        x: page.isPortrait ? (parent.width - width) / 2 : Theme.dp(8)
        anchors {
            bottom: parent.bottom
            bottomMargin: Theme.dp(8)
        }
        spacing: Math.round(menuButton.width * 0.6)

        MapButton {
            radius: Theme.dp(14)
            source: Qt.resolvedUrl("../../icons/help/ic_question_mark.svg")
            onClicked: pageStack.push(Qt.resolvedUrl("HelpPage.qml"))
        }
        MapButton {
            radius: Theme.dp(14)
            source: "image://theme/icon-m-search"
            onClicked: pageStack.push(Qt.resolvedUrl("SearchPage.qml"), { search: search })
        }
        MapButton {
            radius: Theme.dp(14)
            source: Qt.resolvedUrl("../../icons/bookmarks/ic_bookmarks_and_tracks.svg")
            onClicked: pageStack.push(Qt.resolvedUrl("BookmarksPage.qml"))
        }
        MapButton {
            id: menuButton
            radius: Theme.dp(14)
            source: "image://theme/icon-m-menu"
            highlighted: menuPanel.open
            onClicked: menuPanel.open = !menuPanel.open
        }
    }

    // Active search query, kept running on the map like on Android; the cross ends it.
    Rectangle {
        anchors {
            left: layersButton.right
            right: parent.right
            verticalCenter: layersButton.verticalCenter
            leftMargin: Theme.dp(8)
            rightMargin: Theme.dp(8)
        }
        height: layersButton.height
        radius: height / 2
        color: Theme.rgba(Theme.overlayBackgroundColor, Theme.opacityOverlay)
        visible: search.query !== ""

        BackgroundItem {
            anchors {
                left: parent.left
                right: clearSearchButton.left
                top: parent.top
                bottom: parent.bottom
            }
            onClicked: pageStack.push(Qt.resolvedUrl("SearchPage.qml"), { search: search })

            Icon {
                id: searchIcon
                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }
                source: "image://theme/icon-m-search"
            }
            Label {
                anchors {
                    left: searchIcon.right
                    leftMargin: Theme.paddingSmall
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                }
                text: search.query
                truncationMode: TruncationMode.Fade
            }
        }
        IconButton {
            id: clearSearchButton
            anchors {
                right: parent.right
                verticalCenter: parent.verticalCenter
            }
            icon.source: "image://theme/icon-m-cancel"
            onClicked: search.query = ""
        }
    }

    // Lives with the map so the last query and its results come back when search is reopened.
    SearchModel {
        id: search
        highlightColor: Theme.highlightColor
    }

    PlacePagePanel {
        id: placePagePanel
        placePage: map.placePage
    }

    MapPanel {
        id: layersPanel

        Item {
            width: parent.width
            height: closeButton.height

            Label {
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    right: closeButton.left
                    verticalCenter: parent.verticalCenter
                }
                text: qsTr("Map Styles and Layers")
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeLarge
                truncationMode: TruncationMode.Fade
            }

            IconButton {
                id: closeButton
                anchors.right: parent.right
                anchors.rightMargin: Theme.paddingMedium
                icon.source: "image://theme/icon-m-cancel"
                onClicked: layersPanel.open = false
            }
        }

        Row {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x

            Repeater {
                // Order and labels of the Android layers sheet.
                model: [
                    { layer: MapItem.Outdoors, icon: "ic_layers_outdoors", text: qsTr("Outdoors") },
                    { layer: MapItem.Isolines, icon: "ic_layers_isoline", text: qsTr("Contour Lines") },
                    { layer: MapItem.Hiking, icon: "ic_layers_hiking", text: qsTr("Hiking") },
                    { layer: MapItem.Cycling, icon: "ic_layers_cycling", text: qsTr("Cycling") },
                    { layer: MapItem.Subway, icon: "ic_layers_subway", text: qsTr("Subway") }
                ]

                LayerButton {
                    width: parent.width / 5
                    source: Qt.resolvedUrl("../../icons/layers/" + modelData.icon
                                           + (map.darkStyle ? "_night" : "") + ".svg")
                    text: modelData.text
                    checked: (map.enabledLayers & (1 << modelData.layer)) !== 0
                    onClicked: map.setLayerEnabled(modelData.layer, !checked)
                }
            }
        }
    }

    MapPanel {
        id: menuPanel
        spacing: 0

        MenuRow {
            icon: "image://theme/icon-m-cloud-download"
            text: qsTr("Download maps")
            onClicked: {
                menuPanel.open = false
                pageStack.push(Qt.resolvedUrl("MapsPage.qml"))
            }
        }
    }
}
