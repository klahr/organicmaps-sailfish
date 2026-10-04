import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Share 1.0
import app.organicmaps 1.0
import "colors.js" as Colors
import "downloads.js" as Downloads

// Bottom sheet for the selected place, like the Android place page. Not modal, so the map stays usable.
MapPanel {
    id: panel

    // The MapItem.placePage object; the uncreatable C++ type cannot be named as a property type in Qt 5.6.
    property QtObject placePage
    // MapItem.routing, for the route buttons.
    property QtObject routing

    signal addPlaceClicked()
    signal addBusinessClicked()
    // The close button or a swipe closed the panel, not a tap on the map or another selection.
    signal closedByUser()
    // The direction arrow was tapped, to show it fullscreen like on Android.
    signal directionClicked()

    modal: false
    spacing: 0

    // Swiping the panel away deselects the place, and a new selection or a tap on the map updates it.
    onOpenChanged: {
        if (!open && placePage.open) {
            placePage.close()
            closedByUser()
        }
    }
    Connections {
        target: panel.placePage
        onChanged: {
            panel.open = panel.placePage.open
            hoursExpanded = false
            wikiExpanded = false
            flickable.contentY = 0
        }
    }

    property bool hoursExpanded
    property bool wikiExpanded

    // Press and hold copies texts of the page, like on Android.
    function copy(text) {
        Clipboard.text = text
        Notices.show(appInfo.localized("copied_to_clipboard", [text]), Notice.Short, Notice.Center)
    }

    // Long pages scroll inside the sheet, which takes at most about the lower 60% of the map like the
    // half expanded Android sheet.
    SilicaFlickable {
        id: flickable
        width: parent.width
        height: Math.min(content.height, panel.parent.height * 0.6 - actions.height)
        contentHeight: content.height
        clip: true

        Column {
            id: content
            width: parent.width

            Item {
                width: parent.width
                height: header.height + Theme.paddingLarge

                Column {
                    id: header
                    y: Theme.paddingLarge
                    anchors {
                        left: parent.left
                        leftMargin: Theme.horizontalPageMargin
                        right: shareButton.left
                    }

                    Label {
                        width: parent.width - (candidatesButton.visible ? candidatesButton.width : 0)
                        text: placePage.title
                        font.pixelSize: Theme.fontSizeLarge
                        color: Theme.highlightColor
                        wrapMode: Text.Wrap

                        MouseArea {
                            anchors.fill: parent
                            onPressAndHold: panel.copy(placePage.title)
                        }
                        // Several tracks under the tap: choose another one, like the Android title chevron.
                        IconButton {
                            id: candidatesButton
                            anchors {
                                left: parent.right
                                verticalCenter: parent.verticalCenter
                            }
                            visible: placePage.trackCandidates.length > 1
                            icon.source: "image://theme/icon-m-down"
                            onClicked: pageStack.push(candidatesPage)
                        }
                    }
                    Label {
                        width: parent.width
                        visible: text !== ""
                        text: placePage.secondaryTitle
                        color: Theme.highlightColor
                        wrapMode: Text.Wrap
                    }
                    Label {
                        width: parent.width
                        visible: text !== ""
                        text: placePage.subtitle
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.secondaryHighlightColor
                        wrapMode: Text.Wrap
                    }
                    // Android shows the address in the header too.
                    Label {
                        width: parent.width
                        visible: text !== ""
                        text: placePage.address
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.secondaryColor
                        wrapMode: Text.Wrap

                        MouseArea {
                            anchors.fill: parent
                            onPressAndHold: panel.copy(placePage.address)
                        }
                    }
                }
                IconButton {
                    id: shareButton
                    anchors {
                        right: closeButton.left
                        top: closeButton.top
                    }
                    icon.source: "image://theme/icon-m-share"
                    onClicked: shareAction.trigger()
                }
                IconButton {
                    id: closeButton
                    anchors {
                        right: parent.right
                        rightMargin: Theme.paddingMedium
                        top: parent.top
                        topMargin: Theme.paddingMedium
                    }
                    icon.source: "image://theme/icon-m-cancel"
                    onClicked: {
                        placePage.close()
                        panel.closedByUser()
                    }
                }
                // Direction and distance to the place, the arrow turning with the compass like on Android.
                // A tap shows the direction fullscreen.
                Row {
                    id: direction
                    anchors {
                        right: parent.right
                        rightMargin: Theme.horizontalPageMargin
                        top: closeButton.bottom
                    }
                    spacing: Theme.paddingSmall
                    visible: placePage.distance !== ""

                    Image {
                        anchors.verticalCenter: parent.verticalCenter
                        width: Theme.iconSizeSmall
                        height: width
                        sourceSize: Qt.size(width, height)
                        source: "../../icons/placepage/ic_direction_pagepreview.webp"
                        rotation: placePage.azimuth
                        visible: placePage.azimuth >= 0
                    }
                    Label {
                        text: placePage.distance
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.highlightColor
                    }
                }
                MouseArea {
                    anchors {
                        fill: direction
                        margins: -Theme.paddingSmall
                    }
                    enabled: direction.visible
                    onClicked: panel.directionClicked()
                }
            }

            // The list of a bookmark or track with its color, like the Android category row: a tap on the color
            // changes it, a tap on the row moves it to another list.
            ListItem {
                id: categoryRow
                visible: placePage.category !== ""
                contentHeight: Theme.itemSizeSmall
                onClicked: pageStack.push(categoryPage)

                Rectangle {
                    id: colorDot
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    width: Theme.iconSizeSmallPlus
                    height: width
                    radius: width / 2
                    color: placePage.color || "transparent"

                    MouseArea {
                        anchors {
                            fill: parent
                            margins: -Theme.paddingMedium
                        }
                        onClicked: pageStack.push(Qt.resolvedUrl("ColorPickerPage.qml"), {
                            title: appInfo.localized("choose_color"),
                            colors: placePage.colors,
                            chosen: function(colorIndex) { placePage.setColor(colorIndex) }
                        })
                    }
                }
                Label {
                    anchors {
                        left: colorDot.right
                        leftMargin: Theme.paddingLarge
                        right: parent.right
                        rightMargin: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    text: placePage.category
                    truncationMode: TruncationMode.Fade
                    highlighted: categoryRow.highlighted
                }
            }
            // Notes of a bookmark or track and the OpenStreetMap description.
            Repeater {
                model: [placePage.notes, placePage.osmDescription]

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * x
                    visible: modelData !== ""
                    text: modelData
                    textFormat: Text.StyledText
                    wrapMode: Text.Wrap
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.secondaryHighlightColor
                    bottomPadding: Theme.paddingMedium
                    linkColor: Theme.highlightColor
                    onLinkActivated: Qt.openUrlExternally(link)
                }
            }
            // The map of the place isn't downloaded, like the Android download button of the place page.
            ListItem {
                id: countryRow
                readonly property var country: placePage.country
                readonly property bool busy: country.status === CountriesModel.Downloading
                                             || country.status === CountriesModel.InQueue
                                             || country.status === CountriesModel.Applying
                visible: !!country.countryId
                contentHeight: Theme.itemSizeMedium
                onClicked: {
                    if (busy)
                        placePage.cancelCountry()
                    else
                        Downloads.start(pageStack, function() { placePage.downloadCountry() })
                }

                Icon {
                    id: countryIcon
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    visible: !countryRow.busy
                    source: "../../icons/menu/ic_download.svg"
                    sourceSize: Qt.size(Theme.iconSizeMedium, Theme.iconSizeMedium)
                    highlighted: countryRow.highlighted
                }
                ProgressCircle {
                    anchors.fill: countryIcon
                    visible: countryRow.busy
                    value: countryRow.country.progress || 0
                    progressColor: Theme.highlightColor
                    backgroundColor: Theme.rgba(Theme.highlightDimmerColor, 0.5)
                }
                Label {
                    anchors {
                        left: countryIcon.right
                        leftMargin: Theme.paddingLarge
                        right: parent.right
                        rightMargin: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    text: countryRow.busy ? appInfo.localized("cancel_download") + " • " + (countryRow.country.name || "")
                        : appInfo.localized("downloader_download_map") + " • " + (countryRow.country.name || "")
                          + " (" + (countryRow.country.size || "") + ")"
                    truncationMode: TruncationMode.Fade
                    highlighted: countryRow.highlighted
                }
            }

            ElevationProfile {
                visible: placePage.isTrack
                placePage: panel.placePage
            }

            // Routes through a stop, like the Android route row: tap to choose one and show it on the map.
            ListItem {
                id: routesRow
                visible: placePage.routeRefs !== ""
                // At least the height of the other rows, growing when many routes wrap.
                contentHeight: Math.max(Theme.itemSizeMedium, routesLabel.height + 2 * Theme.paddingMedium)
                onClicked: openMenu()

                Icon {
                    id: routesIcon
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: routesLabel.verticalCenter
                    width: Theme.iconSizeMedium
                    height: width
                    sourceSize: Qt.size(width, height)
                    source: "../../icons/placepage/" + (placePage.isTramStop ? "ic_category_tram.svg"
                                                                             : "ic_category_bus.svg")
                    highlighted: routesRow.highlighted
                }
                Label {
                    id: routesLabel
                    anchors {
                        left: routesIcon.right
                        leftMargin: Theme.paddingLarge
                        right: parent.right
                        rightMargin: Theme.horizontalPageMargin
                    }
                    anchors.verticalCenter: parent.verticalCenter
                    text: placePage.routeRefs
                    textFormat: Text.StyledText
                    wrapMode: Text.Wrap
                    highlighted: routesRow.highlighted
                }

                menu: ContextMenu {
                    Repeater {
                        model: placePage.routes

                        MenuItem {
                            text: modelData.label
                            truncationMode: TruncationMode.Fade
                            onClicked: placePage.showRoute(index)

                            // The line color, when the route has one.
                            Rectangle {
                                visible: modelData.color !== ""
                                anchors {
                                    left: parent.left
                                    leftMargin: Theme.paddingMedium
                                    verticalCenter: parent.verticalCenter
                                }
                                width: Theme.paddingSmall
                                height: parent.height * 0.6
                                radius: width / 2
                                color: modelData.color || "transparent"
                            }
                        }
                    }
                }
            }

            // Opening state with the next change, expanding to the full schedule.
            BackgroundItem {
                width: parent.width
                height: hoursColumn.height + 2 * Theme.paddingMedium
                visible: placePage.openingHours !== ""
                onClicked: hoursExpanded = !hoursExpanded

                Icon {
                    id: hoursIcon
                    x: Theme.horizontalPageMargin
                    y: Theme.paddingMedium
                    source: "image://theme/icon-m-clock"
                }
                Column {
                    id: hoursColumn
                    y: Theme.paddingMedium
                    anchors {
                        left: hoursIcon.right
                        leftMargin: Theme.paddingLarge
                        right: expandIcon.left
                    }

                    Label {
                        width: parent.width
                        text: placePage.openTitle !== "" ? placePage.openTitle : placePage.openingHours
                        color: placePage.openState === PlacePage.Open ? Colors.open
                             : placePage.openState === PlacePage.Closed ? Colors.closed : Theme.primaryColor
                        wrapMode: Text.Wrap
                    }
                    Label {
                        width: parent.width
                        visible: text !== ""
                        text: placePage.openDescription
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.secondaryColor
                    }
                    // The week from today, today in bold, like the Android opening hours table.
                    Column {
                        width: parent.width
                        visible: hoursExpanded && placePage.openingSchedule.length > 0
                        topPadding: Theme.paddingSmall

                        Repeater {
                            model: placePage.openingSchedule

                            Row {
                                width: parent.width

                                Label {
                                    width: parent.width * 0.4
                                    text: modelData.days
                                    font.pixelSize: Theme.fontSizeSmall
                                    font.bold: modelData.today
                                    truncationMode: TruncationMode.Fade
                                }
                                Label {
                                    width: parent.width * 0.6
                                    text: modelData.hours
                                    font.pixelSize: Theme.fontSizeSmall
                                    font.bold: modelData.today
                                    wrapMode: Text.Wrap
                                }
                            }
                        }
                    }
                    // Rules that don't fit a weekly table: the OSM opening_hours value, one rule per line.
                    Label {
                        width: parent.width
                        visible: hoursExpanded && placePage.openTitle !== "" && placePage.openingSchedule.length === 0
                        text: placePage.openingHours.split(";").map(function(rule) { return rule.trim() }).join("\n")
                        font.pixelSize: Theme.fontSizeSmall
                        wrapMode: Text.Wrap
                    }
                }
                Icon {
                    id: expandIcon
                    anchors {
                        right: parent.right
                        rightMargin: Theme.horizontalPageMargin
                    }
                    y: Theme.paddingMedium
                    visible: placePage.openTitle !== "" || placePage.openingSchedule.length > 0
                    source: hoursExpanded ? "image://theme/icon-m-up" : "image://theme/icon-m-down"
                }
            }

            // Wikipedia summary, folded like on Android.
            Column {
                width: parent.width
                visible: placePage.wikiDescription !== "" || placePage.wikiUrl !== ""

                MenuRow {
                    icon: "../../icons/placepage/ic_wiki.svg"
                    text: appInfo.localized("read_in_wikipedia")
                    enabled: placePage.wikiUrl !== ""
                    onClicked: Qt.openUrlExternally(placePage.wikiUrl)
                }
                Label {
                    id: wikiLabel
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * x
                    visible: placePage.wikiDescription !== ""
                    text: placePage.wikiDescription
                    textFormat: Text.StyledText
                    wrapMode: Text.Wrap
                    font.pixelSize: Theme.fontSizeSmall
                    maximumLineCount: wikiExpanded ? 10000 : 9
                    elide: Text.ElideRight
                }
                Label {
                    x: Theme.horizontalPageMargin
                    visible: wikiLabel.truncated || wikiExpanded
                    text: wikiExpanded ? appInfo.localized("less") : appInfo.localized("text_more_button")
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeSmall
                    bottomPadding: Theme.paddingMedium

                    MouseArea {
                        anchors.fill: parent
                        onClicked: wikiExpanded = !wikiExpanded
                    }
                }
            }

            Repeater {
                model: placePage.details

                MenuRow {
                    icon: modelData.icon
                    text: modelData.text
                    onClicked: {
                        if (modelData.url !== "")
                            Qt.openUrlExternally(modelData.url)
                        else
                            openMenu()
                    }

                    menu: ContextMenu {
                        MenuItem {
                            text: appInfo.localized("copy_value", [modelData.text])
                            onClicked: Clipboard.text = modelData.text
                        }
                        MenuItem {
                            visible: modelData.url !== "" && modelData.url !== modelData.text
                            text: appInfo.localized("copy_value", [modelData.url])
                            onClicked: Clipboard.text = modelData.url
                        }
                    }
                }
            }
            // Tap switches the format like on Android, press and hold offers every format for copying.
            MenuRow {
                icon: "image://theme/icon-m-whereami"
                text: placePage.coordinates
                onClicked: placePage.nextCoordinatesFormat()

                menu: ContextMenu {
                    Repeater {
                        model: placePage.coordinateValues

                        MenuItem {
                            text: appInfo.localized("copy_value", [modelData])
                            onClicked: Clipboard.text = modelData
                        }
                    }
                }
            }
            MenuRow {
                visible: placePage.isBookmark || placePage.isTrack
                icon: "image://theme/icon-m-edit"
                text: appInfo.localized(placePage.isTrack ? "edit_track" : "placepage_edit_bookmark_button")
                onClicked: pageStack.push(Qt.resolvedUrl("EditBookmarkPage.qml"),
                                          { itemId: placePage.userMarkId, isTrack: placePage.isTrack })
            }
            MenuRow {
                visible: placePage.isTrack
                icon: "image://theme/icon-m-share"
                text: appInfo.localized("export_file_gpx")
                onClicked: bookmarksIO.exportTrack(placePage.userMarkId, BookmarksIO.Gpx)
            }
            // Hands the geo: link to the default handler, e.g. Pure Maps.
            MenuRow {
                icon: "../../icons/placepage/ic_open_in.svg"
                text: appInfo.localized("open_in_app")
                onClicked: Qt.openUrlExternally(placePage.geoUri)
            }
            // Hidden while a route is planned and disabled where the map can't be edited, as on Android.
            MenuRow {
                visible: placePage.canEdit
                enabled: placePage.editable
                opacity: enabled ? 1.0 : Theme.opacityLow
                icon: "image://theme/icon-m-edit"
                text: appInfo.localized("edit_place")
                onClicked: pageStack.push(Qt.resolvedUrl("EditPlacePage.qml"))
            }
            MenuRow {
                visible: placePage.canAddPlace && !routing.active
                enabled: placePage.editable
                opacity: enabled ? 1.0 : Theme.opacityLow
                icon: "image://theme/icon-m-add"
                text: appInfo.localized("placepage_add_place_button")
                onClicked: panel.addPlaceClicked()
            }
            MenuRow {
                visible: placePage.canAddBusiness && !routing.active
                enabled: placePage.editable
                opacity: enabled ? 1.0 : Theme.opacityLow
                icon: "image://theme/icon-m-add"
                text: appInfo.localized("placepage_add_business_button")
                onClicked: panel.addBusinessClicked()
            }
        }

        VerticalScrollDecorator {}
    }

    // Fixed actions below the scrolling content, like the Android place page bar:
    // Route from, Add stop while a route is planned, Save, Route to. A road warning on the route offers to avoid
    // such roads and a route point to be removed instead.
    readonly property bool specialAction: placePage.roadToAvoid !== 0 || picking || placePage.isRoutePoint
    // A route slot waits for a place: this one fills it, like the Android pick buttons.
    readonly property bool picking: routing.pickType >= 0

    PlaceAction {
        width: parent.width
        visible: panel.picking && placePage.roadToAvoid === 0
        icon: routing.pickType === Routing.Start ? "../../icons/routing/ic_route_from.webp"
            : routing.pickType === Routing.Finish ? "../../icons/routing/ic_route_to.webp"
            : "../../icons/routing/ic_route_via.webp"
        text: routing.pickIndex >= 0
              ? appInfo.localized(routing.pickType === Routing.Start ? "change_start_location"
                                : routing.pickType === Routing.Finish ? "change_destination" : "placepage_replace_stop")
              : appInfo.localized(routing.pickType === Routing.Start ? "p2p_from_here"
                                : routing.pickType === Routing.Finish ? "p2p_to_here" : "placepage_add_stop")
        onClicked: routing.pickPlace()
    }
    PlaceAction {
        width: parent.width
        visible: panel.specialAction && !(panel.picking && placePage.roadToAvoid === 0)
        icon: placePage.roadToAvoid === Routing.Toll ? "../../icons/routing/ic_avoid_tolls.webp"
            : placePage.roadToAvoid === Routing.Dirty ? "../../icons/routing/ic_avoid_unpaved.webp"
            : placePage.roadToAvoid === Routing.Ferry ? "../../icons/routing/ic_avoid_ferry.webp"
            : "../../icons/routing/ic_route_remove.svg"
        text: placePage.roadToAvoid === Routing.Toll ? appInfo.localized("avoid_tolls")
            : placePage.roadToAvoid === Routing.Dirty ? appInfo.localized("avoid_unpaved")
            : placePage.roadToAvoid === Routing.Ferry ? appInfo.localized("avoid_ferry")
            : appInfo.localized("placepage_remove_stop")
        onClicked: {
            if (placePage.roadToAvoid !== 0)
                routing.avoidRoad(placePage.roadToAvoid)
            else
                routing.removePlacePoint()
        }
    }
    Row {
        id: actions
        width: parent.width
        visible: !panel.specialAction

        readonly property int count: routing.active ? 4 : 3

        PlaceAction {
            width: actions.width / actions.count
            icon: "../../icons/routing/ic_route_from.webp"
            text: appInfo.localized("p2p_from_here")
            onClicked: routing.routeFromPlace()
        }
        PlaceAction {
            visible: routing.active
            width: actions.width / actions.count
            icon: "../../icons/routing/ic_route_via.webp"
            text: appInfo.localized("placepage_add_stop")
            onClicked: routing.addStopFromPlace()
        }
        PlaceAction {
            width: actions.width / actions.count
            icon: placePage.isBookmark ? "image://theme/icon-m-favorite-selected" : "image://theme/icon-m-favorite"
            text: placePage.isBookmark ? appInfo.localized("delete")
                : appInfo.localized(placePage.canRestoreBookmark ? "restore" : "save")
            onClicked: placePage.toggleBookmark()
        }
        PlaceAction {
            width: actions.width / actions.count
            icon: "../../icons/routing/ic_route_to.webp"
            text: appInfo.localized("p2p_to_here")
            onClicked: routing.routeToPlace()
        }
    }

    // The tracks under the tap.
    Component {
        id: candidatesPage

        Page {
            allowedOrientations: Orientation.All

            SilicaListView {
                anchors.fill: parent
                header: PageHeader {
                    title: appInfo.localized("tracks_title")
                }
                model: placePage.trackCandidates

                delegate: ListItem {
                    highlighted: down || modelData.selected
                    onClicked: {
                        placePage.selectTrackCandidate(index)
                        pageStack.pop()
                    }

                    Rectangle {
                        id: candidateColor
                        x: Theme.horizontalPageMargin
                        anchors.verticalCenter: parent.verticalCenter
                        width: Theme.iconSizeSmall
                        height: width
                        radius: width / 2
                        color: modelData.color
                    }
                    Label {
                        anchors {
                            left: candidateColor.right
                            leftMargin: Theme.paddingLarge
                            right: parent.right
                            rightMargin: Theme.horizontalPageMargin
                            verticalCenter: parent.verticalCenter
                        }
                        text: modelData.title
                        truncationMode: TruncationMode.Fade
                        highlighted: parent.highlighted
                    }
                }
            }
        }
    }

    // Lists to move the bookmark or track to.
    Component {
        id: categoryPage

        Page {
            allowedOrientations: Orientation.All

            SilicaListView {
                anchors.fill: parent
                header: PageHeader {
                    title: appInfo.localized("select_list")
                }
                model: placePage.categories()

                delegate: ListItem {
                    highlighted: down || modelData.name === placePage.category
                    onClicked: {
                        placePage.setCategory(modelData.id)
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
        id: shareAction
        mimeType: "text/plain"
        resources: [{ "type": "text/plain", "data": placePage.shareText, "name": placePage.title }]
    }
}
