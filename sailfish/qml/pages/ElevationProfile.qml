import QtQuick 2.6
import Sailfish.Silica 1.0

// A track's statistics and elevation profile, like the Android elevation profile on the track place page.
// Tapping the chart marks that point of the track on the map.
Column {
    id: root

    // MapItem.placePage.
    property QtObject placePage

    readonly property var profile: placePage.elevationProfile
    readonly property bool hasProfile: profile.length >= 4

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

    Item {
        width: parent.width
        height: Theme.itemSizeHuge * 1.2
        visible: root.hasProfile

        // Altitude range labels on the left, like the Android chart axis.
        Label {
            id: maxLabel
            x: Theme.horizontalPageMargin
            anchors.top: chart.top
            text: placePage.maxElevation
            font.pixelSize: Theme.fontSizeTiny
            color: Theme.secondaryColor
        }
        Label {
            x: Theme.horizontalPageMargin
            anchors.bottom: chart.bottom
            text: placePage.minElevation
            font.pixelSize: Theme.fontSizeTiny
            color: Theme.secondaryColor
        }

        Canvas {
            id: chart

            property real minAltitude
            property real maxAltitude
            readonly property real length: Math.max(1, placePage.trackLength)

            anchors {
                left: maxLabel.right
                leftMargin: Theme.paddingMedium
                right: parent.right
                rightMargin: Theme.horizontalPageMargin
                top: parent.top
                topMargin: Theme.paddingMedium
                bottom: parent.bottom
                bottomMargin: Theme.paddingMedium
            }

            function xOf(distance) { return distance / length * width }
            function yOf(altitude) {
                return height - (altitude - minAltitude) / Math.max(1, maxAltitude - minAltitude) * height
            }

            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                var p = root.profile
                if (p.length < 4)
                    return
                var min = p[1], max = p[1]
                for (var i = 3; i < p.length; i += 2) {
                    min = Math.min(min, p[i])
                    max = Math.max(max, p[i])
                }
                minAltitude = min
                maxAltitude = max

                ctx.beginPath()
                ctx.moveTo(xOf(p[0]), height)
                for (i = 0; i < p.length; i += 2)
                    ctx.lineTo(xOf(p[i]), yOf(p[i + 1]))
                ctx.lineTo(xOf(p[p.length - 2]), height)
                ctx.closePath()
                ctx.fillStyle = Theme.rgba(Theme.highlightColor, 0.2)
                ctx.fill()

                ctx.beginPath()
                ctx.moveTo(xOf(p[0]), yOf(p[1]))
                for (i = 2; i < p.length; i += 2)
                    ctx.lineTo(xOf(p[i]), yOf(p[i + 1]))
                ctx.lineWidth = Theme.dp(2)
                ctx.strokeStyle = Theme.highlightColor
                ctx.stroke()
            }

            Connections {
                target: root
                onProfileChanged: chart.requestPaint()
            }
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()

            // The point chosen on the profile or the map.
            Rectangle {
                visible: placePage.elevationActivePoint >= 0
                x: chart.xOf(placePage.elevationActivePoint) - width / 2
                width: Theme.dp(2)
                height: parent.height
                color: Theme.primaryColor
            }
            // The position, when on the track.
            Rectangle {
                visible: placePage.elevationMyPosition >= 0
                x: chart.xOf(placePage.elevationMyPosition) - width / 2
                y: parent.height - height / 2
                width: Theme.paddingMedium
                height: width
                radius: width / 2
                color: Theme.highlightColor
                border.color: Theme.primaryColor
                border.width: Theme.dp(1)
            }

            MouseArea {
                anchors.fill: parent
                onClicked: placePage.setElevationActivePoint(mouse.x / chart.width * chart.length)
            }
        }
    }
}
