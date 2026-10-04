import QtQuick 2.6
import Sailfish.Silica 1.0

// The app cover shows what matters most right now, with its cover actions: the next turn while navigating, the
// track being recorded, map downloads, a planned route, or else where you are.
CoverBackground {
    id: cover

    // The MapPage, and the app's CountriesModel for countries.
    property Item mapPage
    property QtObject countries
    signal activateRequested()

    readonly property QtObject map: mapPage ? mapPage.mapItem : null
    readonly property QtObject routing: map ? map.routing : null
    readonly property var navigation: routing ? routing.navigation : ({})
    readonly property var position: map ? map.positionInfo : ({})
    readonly property string mode: !map ? "idle"
        : routing.navigating ? "navigation"
        : map.trackRecording ? "recording"
        : countries && countries.downloadInProgress && countries.downloadingName !== "" ? "download"
        : routing.built ? "route"
        : "idle"

    // The app logo and name along the top, like other Sailfish covers.
    Row {
        id: header
        x: Theme.paddingLarge
        y: Theme.paddingMedium
        spacing: Theme.paddingSmall

        Image {
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.iconSizeSmall
            height: width
            sourceSize: Qt.size(width, height)
            source: "../../icons/help/logo.svg"
        }
        Label {
            anchors.verticalCenter: parent.verticalCenter
            text: "Organic Maps"
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
        }
    }

    Column {
        anchors {
            top: header.bottom
            topMargin: Theme.paddingMedium
            left: parent.left
            leftMargin: Theme.paddingLarge
            right: parent.right
            rightMargin: Theme.paddingLarge
        }
        spacing: Theme.paddingSmall

        // Navigation: the turn, its distance and street, and the arrival.
        Icon {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: cover.mode === "navigation"
            width: Theme.iconSizeLarge
            height: width
            sourceSize: Qt.size(width, height)
            source: navigation.turnIcon ? "../../icons/navigation/" + navigation.turnIcon : ""
            color: Theme.primaryColor
        }
        // Recording and downloads: what goes on.
        Icon {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: cover.mode === "recording" || cover.mode === "download"
            width: Theme.iconSizeLarge
            height: width
            sourceSize: Qt.size(width, height)
            source: cover.mode === "recording" ? "../../icons/menu/ic_track_recording_status.svg"
                                               : "image://theme/icon-m-cloud-download"
            color: cover.mode === "recording" ? "#f44336" : Theme.primaryColor
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            visible: text !== ""
            text: {
                switch (cover.mode) {
                case "navigation": return navigation.distanceToTurn || ""
                case "recording": return appInfo.localized("track_recording_title")
                case "download": return Math.round(countries.downloadingProgress * 100) + "%"
                case "route": return routing.summary
                default: return position.address || ""
                }
            }
            font.pixelSize: cover.mode === "idle" || cover.mode === "route" ? Theme.fontSizeMedium : Theme.fontSizeLarge
            font.bold: cover.mode === "navigation" || cover.mode === "download"
            wrapMode: Text.Wrap
            maximumLineCount: 3
            elide: Text.ElideRight
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            visible: text !== ""
            text: {
                switch (cover.mode) {
                case "navigation": return navigation.street || ""
                case "recording": return map.recordingSummary
                case "download": return countries.downloadingName
                case "route":
                    var points = routing.points
                    return points.length > 0 ? "→ " + points[points.length - 1].title : ""
                default: return position.coordinates || ""
                }
            }
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.secondaryColor
            wrapMode: Text.Wrap
            maximumLineCount: 2
            elide: Text.ElideRight
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            visible: text !== ""
            text: {
                switch (cover.mode) {
                case "navigation": return navigation.arrival ? "⚑ " + navigation.arrival : ""
                case "idle":
                    return [position.altitude, position.speed].filter(function(part) { return !!part }).join("   ")
                default: return ""
                }
            }
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryHighlightColor
        }
        // Download progress.
        Rectangle {
            visible: cover.mode === "download"
            width: parent.width
            height: Theme.paddingSmall / 2
            color: Theme.rgba(Theme.primaryColor, 0.2)

            Rectangle {
                width: parent.width * (countries ? countries.downloadingProgress : 0)
                height: parent.height
                color: Theme.highlightColor
            }
        }
    }

    CoverActionList {
        enabled: cover.mode === "navigation"
        CoverAction {
            iconSource: routing && routing.voiceEnabled ? "image://theme/icon-cover-mute" : "image://theme/icon-cover-unmute"
            onTriggered: {
                if (routing.voiceAvailable)
                    routing.voiceEnabled = !routing.voiceEnabled
            }
        }
        CoverAction {
            iconSource: "image://theme/icon-cover-cancel"
            onTriggered: routing.stopNavigation()
        }
    }
    // Stops the recording and saves it under the default name.
    CoverActionList {
        enabled: cover.mode === "recording"
        CoverAction {
            iconSource: "image://theme/icon-cover-favorite"
            onTriggered: map.saveAndStopTrackRecording()
        }
    }
    CoverActionList {
        enabled: cover.mode === "download"
        CoverAction {
            iconSource: "image://theme/icon-cover-cancel"
            onTriggered: countries.cancelAll()
        }
    }
    // Starts navigating when nothing needs confirming first, else opens the route.
    CoverActionList {
        enabled: cover.mode === "route"
        CoverAction {
            iconSource: "image://theme/icon-cover-play"
            onTriggered: {
                if (routing.canStart && routing.startIsMyPosition && routing.disclaimerAccepted)
                    routing.start()
                else
                    cover.activateRequested()
            }
        }
    }
    CoverActionList {
        enabled: cover.mode === "idle"
        CoverAction {
            iconSource: "image://theme/icon-cover-search"
            onTriggered: {
                cover.activateRequested()
                mapPage.openSearchFromCover()
            }
        }
    }
}
