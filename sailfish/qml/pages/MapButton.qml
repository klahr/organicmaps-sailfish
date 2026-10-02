import QtQuick 2.6
import Sailfish.Silica 1.0

// Icon button that stays readable on top of the light map.
Rectangle {
    id: button

    property string source
    property bool highlighted
    property alias busy: busyIndicator.running
    signal clicked

    width: Theme.itemSizeMedium
    height: width
    radius: width / 2
    color: Theme.rgba(Theme.overlayBackgroundColor, Theme.opacityOverlay)

    IconButton {
        id: iconButton
        anchors.centerIn: parent
        icon.source: button.source
        // Theme icons already have this size; SVG files would otherwise render at their nominal size.
        icon.sourceSize: Qt.size(Theme.iconSizeMedium, Theme.iconSizeMedium)
        highlighted: button.highlighted || down
        onClicked: button.clicked()
    }

    BusyIndicator {
        id: busyIndicator
        anchors.centerIn: parent
        size: BusyIndicatorSize.Medium
        running: false
    }
}
