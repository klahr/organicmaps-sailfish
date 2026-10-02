import QtQuick 2.6
import Sailfish.Silica 1.0

// Map layer preview with its name; the outline marks an enabled layer, like on Android.
BackgroundItem {
    id: layerButton

    property alias source: preview.source
    property alias text: label.text
    property bool checked

    height: column.height + 2 * Theme.paddingSmall

    Column {
        id: column
        y: Theme.paddingSmall
        width: parent.width
        spacing: Theme.paddingSmall

        Item {
            anchors.horizontalCenter: parent.horizontalCenter
            // Close to the on-screen size of the Android previews, plus room for the outline.
            width: Theme.itemSizeMedium + 2 * Theme.paddingSmall
            height: width

            Image {
                id: preview
                anchors.fill: parent
                anchors.margins: Theme.paddingSmall
                sourceSize: Qt.size(width, height)
            }

            Rectangle {
                anchors.fill: parent
                radius: Theme.paddingMedium
                color: "transparent"
                border.color: Theme.highlightColor
                border.width: Theme.dp(2)
                visible: layerButton.checked
            }
        }

        Label {
            id: label
            x: Theme.paddingSmall
            width: parent.width - 2 * x
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            maximumLineCount: 2
            font.pixelSize: Theme.fontSizeExtraSmall
            color: layerButton.checked || layerButton.highlighted ? Theme.highlightColor : Theme.primaryColor
        }
    }
}
