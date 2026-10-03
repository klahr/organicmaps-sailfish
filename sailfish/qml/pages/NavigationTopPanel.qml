import QtQuick 2.6
import Sailfish.Silica 1.0

// Next turn while navigating, like layout_nav_top on Android: a turn card in the top left with the
// arrow over its distance, the street along the top, and the turn after it and the speed limit
// under the card, and the lanes under the street.
Item {
    property var navigation

    height: Math.max(turnColumn.height, streetBar.height + lanesBar.height)

    Column {
        id: turnColumn
        spacing: Theme.paddingSmall

        Rectangle {
            width: Theme.itemSizeExtraLarge
            height: turnContent.height + 2 * Theme.paddingMedium
            radius: Theme.paddingMedium
            color: Theme.highlightBackgroundColor

            Column {
                id: turnContent
                anchors.centerIn: parent

                Icon {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: Theme.iconSizeLarge
                    height: width
                    sourceSize: Qt.size(width, height)
                    source: navigation.turnIcon ? "../../icons/navigation/" + navigation.turnIcon : ""
                    color: "white"
                }
                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: navigation.distanceToTurn || ""
                    color: "white"
                    font.pixelSize: Theme.fontSizeLarge
                    font.bold: true
                }
            }
        }
        // The turn after the next one.
        Rectangle {
            visible: !!navigation.nextTurnIcon
            width: Theme.iconSizeMedium + Theme.paddingMedium
            height: width
            radius: Theme.paddingSmall
            color: Theme.rgba(Theme.highlightBackgroundColor, 0.8)

            Icon {
                anchors.centerIn: parent
                width: Theme.iconSizeSmallPlus
                height: width
                sourceSize: Qt.size(width, height)
                source: navigation.nextTurnIcon ? "../../icons/navigation/" + navigation.nextTurnIcon : ""
                color: "white"
            }
        }
        RoadSign {
            visible: !!navigation.speedLimit
            width: Theme.itemSizeSmall
            text: navigation.speedLimit || ""
            fontSize: Theme.fontSizeSmall
        }
    }

    // Street to turn into, along the top.
    Rectangle {
        id: streetBar
        visible: !!navigation.street
        anchors {
            left: turnColumn.right
            leftMargin: Theme.paddingMedium
            right: parent.right
        }
        height: streetLabel.height + 2 * Theme.paddingMedium
        radius: Theme.paddingMedium
        color: Theme.rgba(Theme.overlayBackgroundColor, 0.85)

        Label {
            id: streetLabel
            anchors.centerIn: parent
            width: parent.width - 2 * Theme.paddingMedium
            horizontalAlignment: Text.AlignHCenter
            text: navigation.street || ""
            truncationMode: TruncationMode.Fade
        }
    }

    // Lanes before the turn, the recommended ones highlighted, like LanesView on Android.
    Rectangle {
        id: lanesBar
        readonly property var lanes: navigation.lanes || []

        visible: lanes.length > 0
        anchors {
            top: streetBar.visible ? streetBar.bottom : parent.top
            topMargin: streetBar.visible ? Theme.paddingSmall : 0
            horizontalCenter: streetBar.horizontalCenter
        }
        width: lanesRow.width + 2 * Theme.paddingMedium
        height: visible ? Theme.iconSizeMedium + 2 * Theme.paddingSmall : 0
        radius: Theme.paddingMedium
        color: Theme.rgba(Theme.highlightBackgroundColor, 0.9)

        Row {
            id: lanesRow
            anchors.centerIn: parent

            Repeater {
                model: lanesBar.lanes

                Icon {
                    width: Theme.iconSizeMedium
                    height: width
                    sourceSize: Qt.size(width, height)
                    source: "../../icons/navigation/" + modelData.icon
                    color: "white"
                    opacity: modelData.active ? 1.0 : 0.38
                }
            }
        }
    }
}
