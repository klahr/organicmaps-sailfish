import QtQuick 2.6
import Sailfish.Silica 1.0

// Icon button that stays readable on top of the light map. Sized from the Silica theme, with the icon taking
// about the share of the button it has on Android (32 of 48 dp).
Rectangle {
    id: button

    property string source
    property bool highlighted
    property alias busy: busyIndicator.running
    signal clicked

    width: Theme.itemSizeSmall
    height: width
    radius: width / 2
    color: Theme.rgba(Theme.overlayBackgroundColor, Theme.opacityOverlay)

    IconButton {
        anchors.centerIn: parent
        icon.source: button.source
        // Theme icons already have this size; SVG files would otherwise render at their nominal size.
        icon.sourceSize: Qt.size(Theme.iconSizeSmallPlus, Theme.iconSizeSmallPlus)
        highlighted: button.highlighted || down
        onClicked: button.clicked()
    }

    BusyIndicator {
        id: busyIndicator
        anchors.centerIn: parent
        size: BusyIndicatorSize.Small
        running: false
    }
}
