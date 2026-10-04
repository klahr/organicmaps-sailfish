import QtQuick 2.6
import Sailfish.Silica 1.0

// Elevation profile chart with the altitude range on the left, for tracks and routes. Tapping it reports the
// distance tapped.
Item {
    id: root

    // Distance and altitude pairs in one flat list.
    property var profile: []
    property real length
    property string minLabel
    property string maxLabel
    // Distances of the marked point and of the position, -1 without them.
    property real activePoint: -1
    property real myPosition: -1
    signal pointClicked(real distance)

    width: parent.width
    height: Theme.itemSizeHuge * 1.2
    visible: profile.length >= 4

    // Altitude range labels on the left, like the Android chart axis.
    Label {
        id: maxLabel
        x: Theme.horizontalPageMargin
        anchors.top: chart.top
        text: root.maxLabel
        font.pixelSize: Theme.fontSizeTiny
        color: Theme.secondaryColor
    }
    Label {
        x: Theme.horizontalPageMargin
        anchors.bottom: chart.bottom
        text: root.minLabel
        font.pixelSize: Theme.fontSizeTiny
        color: Theme.secondaryColor
    }

    Canvas {
        id: chart

        property real minAltitude
        property real maxAltitude
        readonly property real length: Math.max(1, root.length)

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
            visible: root.activePoint >= 0
            x: chart.xOf(root.activePoint) - width / 2
            width: Theme.dp(2)
            height: parent.height
            color: Theme.primaryColor
        }
        // The position, when on the track.
        Rectangle {
            visible: root.myPosition >= 0
            x: chart.xOf(root.myPosition) - width / 2
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
            onClicked: root.pointClicked(mouse.x / chart.width * chart.length)
        }
    }
}
