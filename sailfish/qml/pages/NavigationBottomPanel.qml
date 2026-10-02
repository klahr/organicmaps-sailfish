import QtQuick 2.6
import Sailfish.Silica 1.0

// Route progress while navigating, like layout_nav_bottom on Android: time left with the arrival
// time and distance left; a progress line; then voice, settings and the Stop button. The current
// speed is shown on the map instead.
Rectangle {
    id: bottom

    property var navigation
    signal stopClicked
    signal settingsClicked

    height: column.height + Theme.paddingMedium
    color: Theme.rgba(Theme.overlayBackgroundColor, 0.95)

    Column {
        id: column
        width: parent.width
        topPadding: Theme.paddingMedium

        Row {
            width: parent.width
            height: Theme.itemSizeLarge

            // Time left, highlighted, with the arrival time under it.
            Column {
                width: parent.width / 2
                anchors.verticalCenter: parent.verticalCenter

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter

                    Label {
                        id: hoursLabel
                        visible: (navigation.hoursLeft || 0) > 0
                        text: navigation.hoursLeft || ""
                        color: Theme.highlightColor
                        font.pixelSize: Theme.fontSizeExtraLarge
                        font.bold: true
                    }
                    Label {
                        visible: (navigation.hoursLeft || 0) > 0
                        anchors.baseline: hoursLabel.baseline
                        text: (navigation.hourUnits || "") + " "
                        color: Theme.highlightColor
                    }
                    Label {
                        id: minutesLabel
                        text: navigation.minutesLeft !== undefined ? navigation.minutesLeft : ""
                        color: Theme.highlightColor
                        font.pixelSize: Theme.fontSizeExtraLarge
                        font.bold: true
                    }
                    Label {
                        anchors.baseline: minutesLabel.baseline
                        text: navigation.minuteUnits || ""
                        color: Theme.highlightColor
                    }
                }
                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: navigation.arrival || ""
                    color: Theme.secondaryColor
                }
            }
            // Distance left.
            NavigationNumber {
                width: parent.width / 2
                value: navigation.distanceLeftValue || ""
                units: navigation.distanceLeftUnits || ""
            }
        }

        // Completed part of the route.
        Rectangle {
            width: parent.width
            height: Theme.paddingSmall / 2
            color: Theme.rgba(Theme.primaryColor, 0.2)

            Rectangle {
                width: parent.width * (navigation.progress || 0)
                height: parent.height
                color: Theme.highlightColor
            }
        }

        Row {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            height: Theme.itemSizeMedium
            spacing: Theme.paddingLarge

            // Voice instructions need a speech engine, which Sailfish OS doesn't have.
            IconButton {
                anchors.verticalCenter: parent.verticalCenter
                icon.source: "image://theme/icon-m-speaker-mute"
                enabled: false
            }
            IconButton {
                anchors.verticalCenter: parent.verticalCenter
                icon.source: "image://theme/icon-m-setting"
                onClicked: bottom.settingsClicked()
            }
            // Red like the Android Stop button.
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - x
                height: Theme.itemSizeExtraSmall
                radius: Theme.paddingMedium
                color: stopArea.pressed ? "#c62828" : "#f44336"

                Label {
                    anchors.centerIn: parent
                    text: appInfo.localized("navigation_stop_button").toUpperCase()
                    color: "white"
                    font.bold: true
                }
                MouseArea {
                    id: stopArea
                    anchors.fill: parent
                    onClicked: bottom.stopClicked()
                }
            }
        }
    }
}
