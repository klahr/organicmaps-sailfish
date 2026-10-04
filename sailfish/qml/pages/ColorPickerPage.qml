import QtQuick 2.6
import Sailfish.Silica 1.0

// The preset colors; tapping one calls chosen with its index and goes back.
Page {
    id: page

    property string title
    property var chosen

    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: page.title
            }
            ColorGrid {
                size: Theme.itemSizeSmall
                spacing: Theme.paddingLarge
                onColorClicked: {
                    page.chosen(index)
                    pageStack.pop()
                }
            }
        }
    }
}
