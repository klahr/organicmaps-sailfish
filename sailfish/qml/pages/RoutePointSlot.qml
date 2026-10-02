import QtQuick 2.6
import Sailfish.Silica 1.0

// An empty route point slot or the "Add stop" row of the route card.
BackgroundItem {
    id: slot

    property string icon
    property alias text: label.text
    property bool accent

    width: parent.width
    height: Theme.itemSizeSmall

    Icon {
        id: slotIcon
        anchors {
            left: parent.left
            leftMargin: Theme.paddingLarge
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeSmallPlus
        height: width
        sourceSize: Qt.size(width, height)
        source: slot.icon
        opacity: slot.accent ? 1.0 : Theme.opacityHigh
        highlighted: slot.accent || slot.highlighted
    }
    Label {
        id: label
        anchors {
            left: slotIcon.right
            leftMargin: Theme.paddingLarge
            right: parent.right
            rightMargin: Theme.paddingLarge
            verticalCenter: parent.verticalCenter
        }
        font.bold: true
        truncationMode: TruncationMode.Fade
        color: slot.accent || slot.highlighted ? Theme.highlightColor : Theme.secondaryColor
    }
}
