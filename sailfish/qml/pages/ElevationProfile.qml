import QtQuick 2.6
import Sailfish.Silica 1.0

// A track's statistics and elevation profile, like the Android elevation profile on the track place page.
// Tapping the chart marks that point of the track on the map.
Column {
    id: root

    // MapItem.placePage.
    property QtObject placePage

    width: parent.width
    bottomPadding: Theme.paddingMedium

    // Two statistics per row.
    Grid {
        x: Theme.horizontalPageMargin
        width: parent.width - 2 * x
        columns: 2
        rowSpacing: Theme.paddingSmall

        Repeater {
            model: placePage.trackStats

            Column {
                width: parent.width / 2

                Label {
                    width: parent.width
                    text: modelData.label
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                    truncationMode: TruncationMode.Fade
                }
                Label {
                    width: parent.width
                    text: modelData.value
                    color: Theme.highlightColor
                    truncationMode: TruncationMode.Fade
                }
            }
        }
    }

    ElevationChart {
        profile: placePage.elevationProfile
        length: placePage.trackLength
        minLabel: placePage.minElevation
        maxLabel: placePage.maxElevation
        activePoint: placePage.elevationActivePoint
        myPosition: placePage.elevationMyPosition
        onPointClicked: placePage.setElevationActivePoint(distance)
    }
}
