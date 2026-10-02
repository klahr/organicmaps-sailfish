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
    }

    MapButton {
        id: layersButton
        anchors {
            top: parent.top
            left: parent.left
            margins: Theme.dp(8)
        }
        source: Qt.resolvedUrl("../../icons/layers/ic_layers.svg")
        highlighted: layersPanel.open
        onClicked: layersPanel.open = true
    }

    // Positions follow the Android map_buttons_layout_regular.xml; the look stays Silica.
    Column {
        id: rightButtons
        anchors {
            right: parent.right
            bottom: bottomButtons.top
            rightMargin: Theme.dp(8)
            // Gaps measured on the Android app: ~104dp above the bottom row, ~80dp above my position.
            bottomMargin: Theme.dp(104)
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
        // Mirrors location::EMyPositionMode.
        MapButton {
            readonly property int pendingPosition: 0
            readonly property int notFollowNoPosition: 1
            readonly property int follow: 3
            readonly property int followAndRotate: 4

            source: map.myPositionMode === pendingPosition || map.myPositionMode === notFollowNoPosition
                    ? "image://theme/icon-m-gps"
                    : map.myPositionMode === followAndRotate ? "image://theme/icon-m-location"
                                                             : "image://theme/icon-m-whereami"
            highlighted: map.myPositionMode === follow || map.myPositionMode === followAndRotate
            busy: map.myPositionMode === pendingPosition
            opacity: map.myPositionMode === notFollowNoPosition ? Theme.opacityHigh : 1.0
            onClicked: map.switchMyPositionMode()
        }
    }

    // Centered bottom row capped at 300dp like on Android, with its four buttons spread evenly:
    // help, search, bookmarks, menu. Empty slots keep the others in their Android positions.
    Row {
        id: bottomButtons
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: Theme.dp(8)
        }
        width: Math.min(parent.width, Theme.dp(300)) - 2 * Theme.dp(8)
        spacing: (width - 4 * menuButton.width) / 3

        Item {
            // Help.
            width: menuButton.width
            height: 1
        }
        MapButton {
            radius: Theme.dp(14)
            source: "image://theme/icon-m-search"
            onClicked: pageStack.push(Qt.resolvedUrl("SearchPage.qml"), { search: search })
        }
        Item {
            // Bookmarks.
            width: menuButton.width
            height: 1
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
