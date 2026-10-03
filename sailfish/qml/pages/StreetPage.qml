import QtQuick 2.6
import Sailfish.Silica 1.0

// Picks one of the streets near the edited place, or a new one, like the Android StreetFragment.
Page {
    id: page

    property var streets: []
    property string current

    signal selected(string street)

    allowedOrientations: Orientation.All

    function select(street) {
        page.selected(street)
        pageStack.pop()
    }

    SilicaListView {
        anchors.fill: parent
        model: page.streets

        header: Column {
            width: parent.width

            PageHeader {
                title: appInfo.localized("choose_street")
            }
            TextField {
                width: parent.width
                label: appInfo.localized("add_street")
                placeholderText: label
                EnterKey.enabled: text.trim() !== ""
                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: page.select(text.trim())
            }
        }

        delegate: BackgroundItem {
            width: parent.width
            onClicked: page.select(modelData)

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                anchors.verticalCenter: parent.verticalCenter
                text: modelData
                truncationMode: TruncationMode.Fade
                color: highlighted || modelData === page.current ? Theme.highlightColor : Theme.primaryColor
            }
        }

        VerticalScrollDecorator {}
    }
}
