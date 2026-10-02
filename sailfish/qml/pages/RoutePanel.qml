import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0

// Route planning sheet like on Android: router types, the route summary and its points.
// Not modal, so places can still be picked on the map.
MapPanel {
    id: panel

    // MapItem.routing; the uncreatable C++ type cannot be named as a property type in Qt 5.6.
    property QtObject routing

    modal: false
    spacing: 0

    property QtObject placePage

    // Shown while a route is planned, giving way to the place page of a tapped place.
    readonly property bool shouldShow: routing.active && !routing.navigating && !placePage.open
    onShouldShowChanged: open = shouldShow
    Component.onCompleted: open = shouldShow
    // Swiping the panel away ends the route, like its close button.
    onOpenChanged: if (!open && shouldShow) routing.close()

    // Router types in the Android order inside a rounded bar, with the close button at the end.
    Item {
        width: parent.width
        height: Theme.itemSizeMedium

        Rectangle {
            id: routerBar
            anchors {
                left: parent.left
                leftMargin: Theme.horizontalPageMargin
                right: closeButton.left
                verticalCenter: parent.verticalCenter
            }
            height: Theme.itemSizeSmall
            radius: height / 2
            color: Theme.rgba(Theme.primaryColor, 0.08)

            Row {
                anchors.fill: parent

                Repeater {
                    model: [
                        { type: Routing.Vehicle, icon: "ic_car.webp" },
                        { type: Routing.Pedestrian, icon: "ic_pedestrian.webp" },
                        { type: Routing.Transit, icon: "ic_transit.webp" },
                        { type: Routing.Bicycle, icon: "ic_bike.webp" },
                        { type: Routing.Ruler, icon: "ic_ruler_route.svg" }
                    ]

                    MouseArea {
                        readonly property bool selected: routing.routerType === modelData.type

                        width: routerBar.width / 5
                        height: routerBar.height
                        onClicked: routing.routerType = modelData.type

                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: Theme.paddingSmall / 2
                            radius: height / 2
                            visible: parent.selected || parent.pressed
                            color: Theme.rgba(Theme.highlightBackgroundColor, parent.pressed ? 0.5 : 0.3)
                        }
                        Icon {
                            anchors.centerIn: parent
                            source: "../../icons/routing/" + modelData.icon
                            sourceSize: Qt.size(Theme.iconSizeMedium, Theme.iconSizeMedium)
                            color: parent.selected ? Theme.highlightColor : Theme.primaryColor
                        }
                    }
                }
            }
        }
        IconButton {
            id: closeButton
            anchors {
                right: parent.right
                rightMargin: Theme.paddingMedium
                verticalCenter: parent.verticalCenter
            }
            icon.source: "image://theme/icon-m-clear"
            onClicked: routing.close()
        }
    }

    // Summary or progress, with the routing options.
    Item {
        width: parent.width
        height: Math.max(optionsButton.height, statusColumn.height)

        Column {
            id: statusColumn
            anchors {
                left: parent.left
                leftMargin: Theme.horizontalPageMargin
                right: optionsButton.left
                verticalCenter: parent.verticalCenter
            }

            Row {
                spacing: Theme.paddingMedium
                visible: routing.building

                BusyIndicator {
                    size: BusyIndicatorSize.ExtraSmall
                    running: routing.building
                    anchors.verticalCenter: parent.verticalCenter
                }
                Label {
                    text: qsTr("Planning…")
                    color: Theme.secondaryColor
                }
            }
            // "24 min • (walk) 810 m" for transit, like on Android.
            Row {
                visible: routing.built
                spacing: Theme.paddingSmall

                Label {
                    text: routing.summary
                    font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }
                Label {
                    visible: routing.walkingDistance !== ""
                    text: "•"
                    color: Theme.secondaryColor
                    anchors.verticalCenter: parent.verticalCenter
                }
                Icon {
                    visible: routing.walkingDistance !== ""
                    source: "../../icons/routing/ic_20px_route_planning_walk.webp"
                    sourceSize: Qt.size(Theme.iconSizeExtraSmall, Theme.iconSizeExtraSmall)
                    color: Theme.secondaryColor
                    anchors.verticalCenter: parent.verticalCenter
                }
                Label {
                    visible: routing.walkingDistance !== ""
                    text: routing.walkingDistance
                    color: Theme.secondaryColor
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            // Transit legs: walking, then each line with its number in the line color.
            Flow {
                width: parent.width
                visible: routing.transitSteps.length > 0
                spacing: Theme.paddingSmall
                topPadding: Theme.paddingSmall

                Repeater {
                    model: routing.transitSteps

                    Row {
                        spacing: Theme.paddingSmall

                        Label {
                            visible: index > 0
                            text: "•"
                            color: Theme.secondaryColor
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Rectangle {
                            width: stepRow.width + 2 * Theme.paddingSmall
                            height: Theme.iconSizeSmall + Theme.paddingSmall
                            radius: Theme.paddingSmall
                            color: modelData.color !== "" ? modelData.color : Theme.rgba(Theme.primaryColor, 0.15)

                            Row {
                                id: stepRow
                                anchors.centerIn: parent
                                spacing: Theme.paddingSmall

                                Icon {
                                    source: "../../icons/routing/" + modelData.icon
                                    sourceSize: Qt.size(Theme.iconSizeExtraSmall, Theme.iconSizeExtraSmall)
                                    color: modelData.color !== "" ? "white" : Theme.primaryColor
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Label {
                                    visible: modelData.number !== ""
                                    text: modelData.number
                                    color: "white"
                                    font.pixelSize: Theme.fontSizeExtraSmall
                                    font.bold: true
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                        }
                    }
                }
            }
            Label {
                width: parent.width
                visible: text !== ""
                text: routing.errorTitle
                color: Theme.errorColor
                wrapMode: Text.Wrap
            }
            Label {
                width: parent.width
                visible: text !== ""
                text: routing.errorMessage
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
                wrapMode: Text.Wrap
            }
        }
        IconButton {
            id: optionsButton
            anchors {
                right: parent.right
                rightMargin: Theme.paddingMedium
                verticalCenter: parent.verticalCenter
            }
            icon.source: "image://theme/icon-m-setting"
            onClicked: pageStack.push(Qt.resolvedUrl("RoutingOptionsPage.qml"), { routing: routing })
        }
    }

    // Maps the route needs, as offered by the Android routing dialog.
    Button {
        anchors.horizontalCenter: parent.horizontalCenter
        visible: routing.missingMaps.length > 0
        text: appInfo.localized("download") + " (" + routing.missingMapsSize + ")"
        onClicked: routing.downloadMissingMaps()
    }

    // Route points in a card. Like on Android a missing start or finish shows as an empty slot, and
    // stops can be added once both are set. Press and hold a point to reorder or remove it, in place of
    // the Android drag handles. Empty slots and stops are picked like any place: search or the map.
    readonly property bool hasStart: routing.points.length > 0 && routing.points[0].type === Routing.Start
    readonly property bool hasFinish: routing.points.length > 0
                                      && routing.points[routing.points.length - 1].type === Routing.Finish

    Item {
        width: parent.width
        height: card.height + Theme.paddingMedium

        Rectangle {
            id: card
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            height: pointsColumn.height
            radius: Theme.paddingLarge
            color: Theme.rgba(Theme.primaryColor, 0.06)

            Column {
                id: pointsColumn
                width: parent.width

                RoutePointSlot {
                    visible: !panel.hasStart
                    icon: "../../icons/routing/route_point_start.png"
                    text: appInfo.localized("p2p_from_here")
                    onClicked: pageStack.push(Qt.resolvedUrl("SearchPage.qml"), { search: searchModel })
                }

                Repeater {
                    model: routing.points

                    ListItem {
                        id: pointItem

                        width: pointsColumn.width
                        contentHeight: Theme.itemSizeSmall

                        Image {
                            id: pointIcon
                            anchors {
                                left: parent.left
                                leftMargin: Theme.paddingLarge
                                verticalCenter: parent.verticalCenter
                            }
                            width: Theme.iconSizeSmallPlus
                            height: width
                            sourceSize: Qt.size(width, height)
                            source: modelData.isMyPosition ? "../../icons/routing/ic_location_arrow_blue.svg"
                                  : modelData.type === Routing.Start ? "../../icons/routing/route_point_start.png"
                                  : modelData.type === Routing.Finish ? "../../icons/routing/route_point_finish.png"
                                  : "../../icons/routing/route_point_0" + Math.min(index, 9) + ".svg"
                        }
                        Label {
                            anchors {
                                left: pointIcon.right
                                leftMargin: Theme.paddingLarge
                                right: parent.right
                                rightMargin: Theme.paddingLarge
                                verticalCenter: parent.verticalCenter
                            }
                            text: modelData.title
                            font.bold: true
                            truncationMode: TruncationMode.Fade
                            highlighted: pointItem.highlighted
                        }

                        menu: ContextMenu {
                            MenuItem {
                                visible: index > 0
                                text: qsTr("Move up")
                                onClicked: routing.movePoint(index, index - 1)
                            }
                            MenuItem {
                                visible: index < routing.points.length - 1
                                text: qsTr("Move down")
                                onClicked: routing.movePoint(index, index + 1)
                            }
                            MenuItem {
                                visible: modelData.type === Routing.Start && !modelData.isMyPosition
                                text: appInfo.localized("core_my_position")
                                onClicked: routing.setStartToMyPosition()
                            }
                            MenuItem {
                                text: appInfo.localized("delete")
                                onClicked: routing.removePoint(index)
                            }
                        }
                    }
                }

                RoutePointSlot {
                    visible: !panel.hasFinish
                    icon: "../../icons/routing/route_point_finish.png"
                    text: appInfo.localized("p2p_to_here")
                    onClicked: pageStack.push(Qt.resolvedUrl("SearchPage.qml"), { search: searchModel })
                }
                RoutePointSlot {
                    visible: panel.hasStart && panel.hasFinish
                    icon: "image://theme/icon-m-add"
                    text: appInfo.localized("placepage_add_stop")
                    accent: true
                    onClicked: pageStack.push(Qt.resolvedUrl("SearchPage.qml"), { search: searchModel })
                }
            }
        }
    }

    // Bottom bar of the Android route sheet: search, bookmarks, maps and START.
    Row {
        id: bottomBar
        x: Theme.horizontalPageMargin
        width: parent.width - 2 * x
        height: Theme.itemSizeMedium
        spacing: Theme.paddingMedium

        Repeater {
            model: [
                { icon: "image://theme/icon-m-search", page: "SearchPage.qml" },
                { icon: Qt.resolvedUrl("../../icons/bookmarks/ic_bookmarks_and_tracks.svg"), page: "BookmarksPage.qml" },
                { icon: Qt.resolvedUrl("../../icons/menu/ic_download.svg"), page: "MapsPage.qml" }
            ]

            MapButton {
                anchors.verticalCenter: parent.verticalCenter
                radius: Theme.dp(14)
                source: modelData.icon
                onClicked: pageStack.push(Qt.resolvedUrl(modelData.page),
                                          modelData.page === "SearchPage.qml" ? { search: searchModel } : {})
            }
        }
        // Car, walking and bicycle routes can be navigated, as on Android.
        Button {
            anchors.verticalCenter: parent.verticalCenter
            width: bottomBar.width - x
            enabled: routing.canStart
            text: appInfo.localized("p2p_start").toUpperCase()
            onClicked: routing.start()
        }
    }

    // The SearchModel of the map page, for "Add stop".
    property QtObject searchModel
}
