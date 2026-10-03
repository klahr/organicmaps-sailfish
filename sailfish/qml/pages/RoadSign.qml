import QtQuick 2.6
import Sailfish.Silica 1.0

// A round red-bordered road sign with a number: the current speed or the speed limit, like on Android.
Rectangle {
    property alias text: label.text
    property alias fontSize: label.font.pixelSize

    height: width
    radius: width / 2
    color: "white"
    border.color: "#e53935"
    border.width: width * 0.1

    Label {
        id: label
        anchors.centerIn: parent
        color: "black"
        font.bold: true
    }
}
