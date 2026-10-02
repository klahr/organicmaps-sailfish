import QtQuick 2.6
import Sailfish.Silica 1.0

// A large number over its units in the navigation sheet.
Column {
    property alias value: valueLabel.text
    property alias units: unitsLabel.text

    anchors.verticalCenter: parent.verticalCenter

    Label {
        id: valueLabel
        anchors.horizontalCenter: parent.horizontalCenter
        font.pixelSize: Theme.fontSizeExtraLarge
        font.bold: true
    }
    Label {
        id: unitsLabel
        anchors.horizontalCenter: parent.horizontalCenter
        color: Theme.secondaryColor
    }
}
