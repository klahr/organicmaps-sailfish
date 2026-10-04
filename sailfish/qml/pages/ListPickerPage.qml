import QtQuick 2.6
import Sailfish.Silica 1.0

// Picks one of items, given as {name, color, selected}: picked is called with its index, then the page goes back.
// It opens at the selected item.
// addText and addAction offer another choice in the pull-down menu, like "Add a New List".
Page {
    id: page

    property string title
    property var items: []
    property var picked
    property string addText
    property var addAction

    allowedOrientations: Orientation.All

    SilicaListView {
        anchors.fill: parent
        header: PageHeader {
            title: page.title
        }
        model: page.items

        PullDownMenu {
            visible: !!page.addAction
            MenuItem {
                text: page.addText
                onClicked: page.addAction()
            }
        }

        delegate: ListItem {
            highlighted: down || !!modelData.selected
            onClicked: {
                page.picked(index)
                pageStack.pop()
            }

            ColorDot {
                id: colorDot
                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                visible: !!modelData.color
                width: visible ? Theme.iconSizeSmall : 0
                color: modelData.color || "transparent"
            }
            Label {
                anchors {
                    left: colorDot.right
                    leftMargin: colorDot.visible ? Theme.paddingLarge : Theme.horizontalPageMargin
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                text: modelData.name
                truncationMode: TruncationMode.Fade
                highlighted: parent.highlighted
            }
        }

        Component.onCompleted: {
            for (var i = 0; i < page.items.length; ++i) {
                if (page.items[i].selected) {
                    positionViewAtIndex(i, ListView.Center)
                    break
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
