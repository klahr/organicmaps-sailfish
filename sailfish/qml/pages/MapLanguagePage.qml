import QtQuick 2.6
import Sailfish.Silica 1.0

// Language of the map labels, from the same list as on Android.
Page {
    allowedOrientations: Orientation.All

    SilicaListView {
        id: listView
        anchors.fill: parent
        model: appSettings.mapLanguages

        header: PageHeader {
            title: appInfo.localized("change_map_locale")
        }

        delegate: BackgroundItem {
            readonly property bool current: modelData.code === appSettings.mapLanguage

            height: Theme.itemSizeSmall
            onClicked: {
                appSettings.mapLanguage = modelData.code
                pageStack.pop()
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                anchors.verticalCenter: parent.verticalCenter
                text: modelData.name
                color: current || parent.highlighted ? Theme.highlightColor : Theme.primaryColor
                truncationMode: TruncationMode.Fade
            }
        }

        // Start at the current language.
        Component.onCompleted: {
            for (var i = 0; i < appSettings.mapLanguages.length; ++i) {
                if (appSettings.mapLanguages[i].code === appSettings.mapLanguage) {
                    positionViewAtIndex(i, ListView.Center)
                    break
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
