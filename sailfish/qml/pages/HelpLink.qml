import QtQuick 2.6
import Sailfish.Silica 1.0

// A link row on the help page, opened in the browser.
BackgroundItem {
    property alias text: label.text
    property string url

    width: parent.width
    height: Theme.itemSizeSmall
    onClicked: Qt.openUrlExternally(url)

    Label {
        id: label
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        color: parent.highlighted ? Theme.highlightColor : Theme.primaryColor
        truncationMode: TruncationMode.Fade
    }
}
