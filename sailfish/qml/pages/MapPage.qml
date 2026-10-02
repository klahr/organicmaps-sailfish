import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Share 1.0
import Nemo.KeepAlive 1.2
import app.organicmaps 1.0

Page {
    id: page
    objectName: "mapPage"
    allowedOrientations: Orientation.All
    backNavigation: false

    // Light or dark map following the appearance setting; Auto follows the Sailfish ambience.
    readonly property bool mapIsDark: appSettings.mapAppearance === AppSettings.AppearanceAuto
                                      ? Theme.colorScheme === Theme.LightOnDark
                                      : appSettings.mapAppearance === AppSettings.AppearanceDark
    onMapIsDarkChanged: appSettings.applyMapAppearance(Theme.colorScheme === Theme.LightOnDark)
    Component.onCompleted: appSettings.applyMapAppearance(Theme.colorScheme === Theme.LightOnDark)

    // A recording keeps the device awake like the Android foreground service; keep screen on is a setting.
    KeepAlive {
        enabled: map.trackRecording
    }
    DisplayBlanking {
        preventBlanking: appSettings.keepScreenOn && Qt.application.active
    }

    ShareAction {
        id: shareLocationAction
        mimeType: "text/plain"
    }

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
            visible: appSettings.zoomButtons
            source: "image://theme/icon-m-add"
            onClicked: map.zoomIn()
        }
        MapButton {
            visible: appSettings.zoomButtons
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

    // Blinking status in the top right corner while a track records, like on Android. Tapping it stops.
    MapButton {
        id: recordingButton
        anchors {
            top: parent.top
            right: parent.right
            margins: Theme.dp(8)
        }
        visible: map.trackRecording
        highlighted: true
        source: Qt.resolvedUrl("../../icons/menu/ic_track_recording_status.svg")
        onClicked: page.stopTrackRecording()

        SequentialAnimation on opacity {
            running: recordingButton.visible && Qt.application.active
            loops: Animation.Infinite
            NumberAnimation { to: 0.4; duration: 800 }
            NumberAnimation { to: 1.0; duration: 800 }
        }
    }

    function stopTrackRecording() {
        if (map.isTrackRecordingEmpty())
            map.stopTrackRecording("")
        else
            pageStack.push(Qt.resolvedUrl("SaveTrackDialog.qml"), { map: map })
    }

    // Active search query, kept running on the map like on Android; the cross ends it.
    Rectangle {
        anchors {
            left: layersButton.right
            right: recordingButton.visible ? recordingButton.left : parent.right
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
                id: layersRepeater
                // Order and labels of the Android layers sheet; Satellite once a tile server is set.
                model: {
                    var layers = [
                        { layer: MapItem.Outdoors, icon: "ic_layers_outdoors", text: appInfo.localized("button_layer_outdoor") },
                        { layer: MapItem.Isolines, icon: "ic_layers_isoline", text: appInfo.localized("button_layer_isolines") },
                        { layer: MapItem.Hiking, icon: "ic_layers_hiking", text: appInfo.localized("button_layer_hiking") },
                        { layer: MapItem.Cycling, icon: "ic_layers_cycling", text: appInfo.localized("button_layer_cycling") },
                        { layer: MapItem.Subway, icon: "ic_layers_subway", text: appInfo.localized("button_layer_subway") }
                    ]
                    if (appSettings.bgTilesUrl !== "")
                        layers.push({ layer: MapItem.Satellite, icon: "ic_layers_satellite",
                                      text: appInfo.localized("button_layer_satellite") })
                    return layers
                }

                LayerButton {
                    width: parent.width / layersRepeater.count
                    source: Qt.resolvedUrl("../../icons/layers/" + modelData.icon
                                           + (page.mapIsDark ? "_night" : "") + ".svg")
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

        // Entries and order of the Android main menu. Adding places to OpenStreetMap needs the editor.
        MenuRow {
            icon: "../../icons/menu/ic_download.svg"
            text: appInfo.localized("download_maps")
            onClicked: {
                menuPanel.open = false
                pageStack.push(Qt.resolvedUrl("MapsPage.qml"))
            }
        }
        MenuRow {
            visible: appSettings.donateUrl !== ""
            icon: "../../icons/menu/ic_donate.svg"
            text: appInfo.localized("donate")
            onClicked: {
                menuPanel.open = false
                Qt.openUrlExternally(appSettings.donateUrl)
            }
        }
        MenuRow {
            icon: "../../icons/menu/ic_settings.svg"
            text: appInfo.localized("settings")
            onClicked: {
                menuPanel.open = false
                pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }
        }
        MenuRow {
            icon: map.trackRecording ? "../../icons/menu/ic_track_recording_on.svg"
                                     : "../../icons/menu/ic_track_recording_off.svg"
            text: map.trackRecording ? appInfo.localized("stop_track_recording")
                                     : appInfo.localized("start_track_recording")
            onClicked: {
                menuPanel.open = false
                if (map.trackRecording)
                    page.stopTrackRecording()
                else
                    map.startTrackRecording()
            }
        }
        MenuRow {
            icon: "../../icons/menu/ic_share.svg"
            text: appInfo.localized("share_my_location")
            enabled: map.myPositionMode > 1
            opacity: enabled ? 1.0 : Theme.opacityLow
            onClicked: {
                menuPanel.open = false
                shareLocationAction.resources = [{ "type": "text/plain", "data": map.myPositionShareText(),
                                                   "name": appInfo.localized("share_my_location") }]
                shareLocationAction.trigger()
            }
        }
    }
}
