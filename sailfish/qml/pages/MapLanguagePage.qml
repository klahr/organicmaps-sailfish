import QtQuick 2.6
import Sailfish.Silica 1.0

// Language of the map labels, from the same list as on Android.
Page {
    allowedOrientations: Orientation.All

    SilicaListView {
        anchors.fill: parent
        model: appSettings.mapLanguages

        header: PageHeader {
            title: appInfo.localized("change_map_locale")
        }

        delegate: TextRow {
            text: modelData.name
            current: modelData.code === appSettings.mapLanguage
            onClicked: {
                appSettings.mapLanguage = modelData.code
                pageStack.pop()
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
