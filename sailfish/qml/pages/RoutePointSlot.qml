import QtQuick 2.6
import Sailfish.Silica 1.0

// A row of the route card: a route point, an empty start or finish slot, or the "Add stop" row.
ListItem {
    id: slot

    property string icon
    property alias text: label.text
    // An empty slot is dimmed, the "Add stop" row is accented.
    property bool empty
    property bool accent

    width: parent.width
    contentHeight: Theme.itemSizeSmall

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
        opacity: slot.empty ? Theme.opacityHigh : 1.0
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
        color: slot.accent || slot.highlighted ? Theme.highlightColor
                                               : slot.empty ? Theme.secondaryColor : Theme.primaryColor
    }
}
