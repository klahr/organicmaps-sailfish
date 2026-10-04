import QtQuick 2.6
import Sailfish.Silica 1.0

// The recording so far, like the Android track recording place page: distance and time, the elevation
// profile, and Delete or Save to end it.
MapPanel {
    id: panel

    // The MapItem.
    property QtObject map
    signal saveClicked()

    modal: false
    spacing: 0

    Connections {
        target: panel.map
        onTrackRecordingChanged: if (!panel.map.trackRecording) panel.open = false
    }

    Item {
        width: parent.width
        height: header.height + 2 * Theme.paddingLarge

        Column {
            id: header
            anchors {
                left: parent.left
                leftMargin: Theme.horizontalPageMargin
                right: closeButton.left
                verticalCenter: parent.verticalCenter
            }

            Label {
                width: parent.width
                text: appInfo.localized("track_recording_title")
                font.pixelSize: Theme.fontSizeLarge
                color: Theme.highlightColor
                truncationMode: TruncationMode.Fade
            }
            Label {
                width: parent.width
                text: panel.map.recordingSummary
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
            }
        }
        IconButton {
            id: closeButton
            anchors {
                right: parent.right
                rightMargin: Theme.paddingMedium
                verticalCenter: parent.verticalCenter
            }
            icon.source: "image://theme/icon-m-cancel"
            onClicked: panel.open = false
        }
    }

    ElevationChart {
        height: Theme.itemSizeExtraLarge
        profile: panel.map.recordingProfile
        length: panel.map.recordingLength
        minLabel: panel.map.recordingMinElevation
        maxLabel: panel.map.recordingMaxElevation
    }

    Row {
        width: parent.width

        PlaceAction {
            width: parent.width / 2
            icon: "image://theme/icon-m-delete"
            text: appInfo.localized("delete")
            onClicked: Remorse.popupAction(panel.parent, appInfo.localized("delete"), function() {
                panel.map.stopTrackRecording("")
            })
        }
        PlaceAction {
            width: parent.width / 2
            enabled: panel.map.recordingLength > 0
            opacity: enabled ? 1.0 : Theme.opacityLow
            icon: "image://theme/icon-m-device-download"
            text: appInfo.localized("save")
            onClicked: panel.saveClicked()
        }
    }
}
