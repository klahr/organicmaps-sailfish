import QtQuick 2.6
import Sailfish.Silica 1.0

// Icon and label row, e.g. of the map page menu panel or the place page details; it can have a menu.
ListItem {
    id: row

    property string icon
    property string text

    width: parent.width
    contentHeight: Theme.itemSizeMedium

    Icon {
        id: rowIcon
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        source: row.icon
        // Theme icons already have this size; SVG files would otherwise render at their nominal size.
        sourceSize: Qt.size(Theme.iconSizeMedium, Theme.iconSizeMedium)
        highlighted: row.highlighted
    }

    Label {
        anchors {
            left: rowIcon.right
            leftMargin: Theme.paddingLarge
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        text: row.text
        truncationMode: TruncationMode.Fade
        highlighted: row.highlighted
    }
}
