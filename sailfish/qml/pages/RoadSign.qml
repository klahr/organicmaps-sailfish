import QtQuick 2.6
import Sailfish.Silica 1.0

// A round red-bordered road sign with a number: the current speed or the speed limit, like on Android.
Rectangle {
    property alias text: label.text
    property alias fontSize: label.font.pixelSize
    // Red with white text, like the Android speed view over a speed camera limit.
    property bool alert

    height: width
    radius: width / 2
    color: alert ? "#e53935" : "white"
    border.color: "#e53935"
    border.width: width * 0.1

    Label {
        id: label
        anchors.centerIn: parent
        color: alert ? "white" : "black"
        font.bold: true
    }
}
