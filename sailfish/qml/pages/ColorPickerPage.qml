import QtQuick 2.6
import Sailfish.Silica 1.0

// The preset colors; tapping one calls chosen with its index and goes back.
Page {
    id: page

    property string title
    property var colors: []
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
            Grid {
                id: grid

                readonly property real size: Theme.itemSizeSmall

                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                spacing: Theme.paddingLarge
                columns: Math.max(1, Math.floor((width + spacing) / (size + spacing)))

                Repeater {
                    model: page.colors

                    Rectangle {
                        width: grid.size
                        height: width
                        radius: width / 2
                        color: modelData

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                page.chosen(index)
                                pageStack.pop()
                            }
                        }
                    }
                }
            }
        }
    }
}
