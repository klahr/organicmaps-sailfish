import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Share 1.0
import Nemo.KeepAlive 1.2
import app.organicmaps 1.0
import "downloads.js" as Downloads

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
        enabled: map.trackRecording || map.routing.navigating
    }
    // The screen stays on while navigating, as on Android.
    DisplayBlanking {
        preventBlanking: (appSettings.keepScreenOn || map.routing.navigating) && Qt.application.active
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
        // Routes are fitted into the map above the open sheets.
        viewportBottomInset: page.navigating ? navigationBottomPanel.height
                                             : Math.max(placePagePanel.visibleSize, routePanel.visibleSize)
    }

    readonly property bool navigating: map.routing.navigating
    readonly property bool choosingPosition: map.choosingPosition

    function openSearch() {
        var searchPage = pageStack.push(Qt.resolvedUrl("SearchPage.qml"), { search: search })
        searchPage.resultActivated.connect(function() {
            page.searchResultTitle = ""
            page.placeFromSearch = true
        })
    }

    // The place page shows a search result: closing it returns to the results, like on Android. Another
    // selection or closing it with a tap on the map forgets it.
    property bool placeFromSearch
    property string searchResultTitle
    Connections {
        target: map.placePage
        onChanged: {
            if (!page.placeFromSearch)
                return
            if (!map.placePage.open)
                forgetSearchResult.restart()
            else if (page.searchResultTitle === "")
                page.searchResultTitle = map.placePage.title
            else if (map.placePage.title !== page.searchResultTitle)
                page.placeFromSearch = false
        }
    }
    // The close button and swipe report closedByUser right after the place page closes; anything else is a
    // tap on the map.
    Timer {
        id: forgetSearchResult
        interval: 100
        onTriggered: page.placeFromSearch = false
    }
    Connections {
        target: urlHandler
        onRouteRequested: {
            pageStack.pop(page, PageStackAction.Immediate)
            map.routing.planRoute(routerType, points)
        }
        onSearchRequested: {
            pageStack.pop(page, PageStackAction.Immediate)
            search.query = query
            page.openSearch()
        }
    }
    function openBookmarks() {
        pageStack.push(Qt.resolvedUrl("BookmarksPage.qml"))
    }

    NavigationTopPanel {
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            margins: Theme.dp(8)
            // Clear the camera notch in portrait, like the Silica PageHeader does.
            topMargin: Theme.dp(8) + (page.orientation === Orientation.Portrait ? Screen.topCutout.height : 0)
        }
        visible: page.navigating
        navigation: map.routing.navigation
    }

    MapButton {
        visible: !page.navigating && !page.choosingPosition
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
        visible: !page.choosingPosition
        anchors {
            right: parent.right
            rightMargin: Theme.dp(8)
            // Portrait: ~104dp above the bottom row, measured on Android. Landscape: my position is level
            // with the row in the bottom right corner.
            bottom: page.navigating ? (page.isPortrait ? navigationBottomPanel.top : parent.bottom)
                  : page.isPortrait ? bottomButtons.top : parent.bottom
            bottomMargin: page.navigating ? Theme.dp(8)
                        : page.isPortrait ? Theme.dp(104) : bottomButtons.anchors.bottomMargin
        }
        spacing: Theme.dp(8)
        // Stay above the place page, as on Android.
        transform: Translate {
            y: -Math.max(0, Math.max(placePagePanel.visibleSize, routePanel.visibleSize)
                               - (page.height - rightButtons.y - rightButtons.height) + Theme.dp(8))
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
        // The Android icons of the MapItem.MyPositionMode values. The core cycles the modes: a tap centers
        // north up, further taps toggle rotating with the heading, panning the map stops following.
        MapButton {
            readonly property var icons: ["", "ic_location_off", "ic_not_follow", "ic_follow", "ic_follow_and_rotate"]

            // While searching only the spinner shows, like the rotating ring on Android.
            source: map.myPositionMode === MapItem.PendingPosition
                    ? "" : Qt.resolvedUrl("../../icons/myposition/" + icons[map.myPositionMode] + ".svg")
            highlighted: map.myPositionMode === MapItem.Follow || map.myPositionMode === MapItem.FollowAndRotate
            busy: map.myPositionMode === MapItem.PendingPosition
            onClicked: map.switchMyPositionMode()
        }
    }

    // Bottom row like on Android: help, search, bookmarks, menu. Centered in portrait, left aligned in
    // landscape; the gaps are about 0.6 of a button, as measured on the Android app.
    Row {
        id: bottomButtons
        visible: !page.navigating && !page.choosingPosition
        // Positioned by x: switching between left and horizontalCenter anchors on rotation can leave both set.
        x: page.isPortrait ? (parent.width - width) / 2 : Theme.dp(8)
        anchors {
            bottom: parent.bottom
            bottomMargin: Theme.dp(8)
        }
        spacing: Math.round(menuButton.width * 0.6)

        MapButton {
            radius: Theme.dp(14)
            source: Qt.resolvedUrl("../../icons/help/logo.svg")
            onClicked: pageStack.push(Qt.resolvedUrl("HelpPage.qml"))
        }
        MapButton {
            radius: Theme.dp(14)
            source: "image://theme/icon-m-search"
            onClicked: page.openSearch()
        }
        MapButton {
            radius: Theme.dp(14)
            source: Qt.resolvedUrl("../../icons/bookmarks/ic_bookmarks_and_tracks.svg")
            onClicked: page.openBookmarks()
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
        // The navigation panel takes the top while navigating.
        visible: map.trackRecording && !page.navigating
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

    // Full width in portrait; a card in the bottom left corner in landscape, as on Android.
    NavigationBottomPanel {
        id: navigationBottomPanel
        anchors {
            left: parent.left
            bottom: parent.bottom
        }
        width: page.isPortrait ? parent.width : Math.round(parent.width * 0.4)
        radius: page.isPortrait ? 0 : Theme.paddingLarge
        visible: page.navigating
        navigation: map.routing.navigation
        routing: map.routing
        onStopClicked: map.routing.stopNavigation()
        onSettingsClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"), { mapPageRouting: map.routing })
        onVoiceSettingsClicked: pageStack.push(Qt.resolvedUrl("VoicePage.qml"), { routing: map.routing })
    }

    // Search and bookmarks stay at hand on the left while navigating, as on Android, with the
    // current speed under them as a road sign.
    Column {
        anchors {
            left: parent.left
            bottom: navigationBottomPanel.top
            margins: Theme.dp(8)
        }
        visible: page.navigating
        spacing: Theme.dp(8)

        MapButton {
            source: "image://theme/icon-m-search"
            onClicked: page.openSearch()
        }
        MapButton {
            source: Qt.resolvedUrl("../../icons/bookmarks/ic_bookmarks_and_tracks.svg")
            onClicked: page.openBookmarks()
        }
        RoadSign {
            width: Theme.itemSizeSmall * 1.2
            text: map.routing.navigation.speed || "0"
            alert: !!map.routing.navigation.speedCamLimitExceeded
            fontSize: Theme.fontSizeExtraLarge
        }
    }

    // The map of the region in the middle of the map isn't downloaded: its name and size with a download
    // button, then the progress, like the Android on-map downloader.
    Rectangle {
        id: onMapDownloader
        readonly property var country: map.currentCountry
        readonly property bool busy: country.status === CountriesModel.Downloading
                                     || country.status === CountriesModel.InQueue
                                     || country.status === CountriesModel.Applying

        visible: !!country.countryId && !page.navigating && !page.choosingPosition
                 && placePagePanel.visibleSize === 0 && routePanel.visibleSize === 0
        anchors.centerIn: parent
        width: Math.min(parent.width - 2 * Theme.horizontalPageMargin, Theme.itemSizeHuge * 3)
        height: downloaderColumn.height + 2 * Theme.paddingLarge
        radius: Theme.paddingLarge
        color: Theme.rgba(Theme.overlayBackgroundColor, 0.9)

        Column {
            id: downloaderColumn
            anchors.centerIn: parent
            width: parent.width - 2 * Theme.paddingLarge
            spacing: Theme.paddingMedium

            Label {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: onMapDownloader.country.name || ""
                font.pixelSize: Theme.fontSizeLarge
                color: Theme.highlightColor
                wrapMode: Text.Wrap
            }
            Label {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: onMapDownloader.country.status === CountriesModel.Error
                      ? appInfo.localized("country_status_download_failed")
                      : onMapDownloader.country.size || ""
                color: onMapDownloader.country.status === CountriesModel.Error ? Theme.errorColor
                                                                               : Theme.secondaryHighlightColor
            }
            ProgressBar {
                width: parent.width
                visible: onMapDownloader.busy
                indeterminate: onMapDownloader.country.status !== CountriesModel.Downloading
                value: onMapDownloader.country.progress || 0
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: onMapDownloader.busy ? appInfo.localized("cancel")
                    : onMapDownloader.country.status === CountriesModel.Error ? appInfo.localized("downloader_retry")
                    : appInfo.localized("downloader_download_map")
                onClicked: {
                    if (onMapDownloader.busy)
                        map.cancelCurrentCountry()
                    else
                        Downloads.start(pageStack, function() { map.downloadCurrentCountry() })
                }
            }
        }
    }

    // "Add Place to OpenStreetMap": the core draws the cross in the middle of the map, this bar
    // explains it and leads on to the category, like the Android point chooser.
    Rectangle {
        id: positionChooser
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        height: chooserColumn.height + 2 * Theme.paddingMedium
                + (page.orientation === Orientation.Portrait ? Screen.topCutout.height : 0)
        visible: page.choosingPosition
        color: Theme.overlayBackgroundColor

        property bool invalidPosition

        onVisibleChanged: invalidPosition = false

        PanelBackground {
            anchors.fill: parent
        }

        IconButton {
            id: chooserCancel
            anchors {
                left: parent.left
                verticalCenter: chooserColumn.verticalCenter
            }
            icon.source: "image://theme/icon-m-cancel"
            onClicked: map.stopChoosingPosition()
        }

        Column {
            id: chooserColumn
            anchors {
                left: chooserCancel.right
                right: chooserDone.left
                bottom: parent.bottom
                bottomMargin: Theme.paddingMedium
            }
            spacing: Theme.paddingSmall

            Label {
                width: parent.width
                text: appInfo.localized("editor_add_select_location")
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeLarge
                truncationMode: TruncationMode.Fade
            }
            Label {
                width: parent.width
                text: positionChooser.invalidPosition ? appInfo.localized("message_invalid_feature_position")
                                                      : appInfo.localized("editor_focus_map_on_location")
                color: positionChooser.invalidPosition ? Theme.errorColor : Theme.secondaryHighlightColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.Wrap
            }
        }

        IconButton {
            id: chooserDone
            anchors {
                right: parent.right
                verticalCenter: chooserColumn.verticalCenter
            }
            icon.source: "image://theme/icon-m-acknowledge"
            onClicked: {
                var position = map.confirmChosenPosition()
                positionChooser.invalidPosition = position.length === 0
                if (!positionChooser.invalidPosition)
                    pageStack.push(Qt.resolvedUrl("CategoryPage.qml"), { lat: position[0], lon: position[1] })
            }
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
        routing: map.routing
        onAddPlaceClicked: map.startChoosingPosition(false)
        onAddBusinessClicked: map.startChoosingPosition(true)
        onDirectionClicked: directionOverlay.shown = true
        onClosedByUser: {
            if (page.placeFromSearch) {
                forgetSearchResult.stop()
                page.placeFromSearch = false
                page.openSearch()
            }
        }
    }

    RoutePanel {
        id: routePanel
        routing: map.routing
        placePage: map.placePage
        onSearchClicked: page.openSearch()
        onBookmarksClicked: page.openBookmarks()
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
                text: appInfo.localized("layers_title")
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
                        { layer: MapItem.Subway, icon: "ic_layers_subway", text: appInfo.localized("button_layer_subway") },
                        { layer: MapItem.Isolines, icon: "ic_layers_isoline", text: appInfo.localized("button_layer_isolines") },
                        { layer: MapItem.Outdoors, icon: "ic_layers_outdoors", text: appInfo.localized("button_layer_outdoor") },
                        { layer: MapItem.Hiking, icon: "ic_layers_hiking", text: appInfo.localized("button_layer_hiking") },
                        { layer: MapItem.Cycling, icon: "ic_layers_cycling", text: appInfo.localized("button_layer_cycling") }
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

        // Entries and order of the Android main menu.
        MenuRow {
            icon: "image://theme/icon-m-add"
            text: appInfo.localized("placepage_add_place_button")
            onClicked: {
                menuPanel.open = false
                map.startChoosingPosition()
            }
        }
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
                pageStack.push(Qt.resolvedUrl("SettingsPage.qml"), { mapPageRouting: map.routing })
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
            enabled: map.myPositionMode !== MapItem.PendingPosition
                     && map.myPositionMode !== MapItem.NotFollowNoPosition
            opacity: enabled ? 1.0 : Theme.opacityLow
            onClicked: {
                menuPanel.open = false
                shareLocationAction.resources = [{ "type": "text/plain", "data": map.myPositionShareText(),
                                                   "name": appInfo.localized("share_my_location") }]
                shareLocationAction.trigger()
            }
        }
    }

    // Last, to cover the map and all panels.
    DirectionOverlay {
        id: directionOverlay
        placePage: map.placePage
    }
}
