import QtQuick 2.6
import Sailfish.Silica 1.0

// "Add a language" of the place editor, like the Android LanguagesFragment.
Page {
    id: page

    // {code, language} entries to choose from.
    property var languages

    signal selected(int code, string language)

    allowedOrientations: Orientation.All

    SilicaListView {
        anchors.fill: parent
        model: page.languages

        header: PageHeader {
            title: appInfo.localized("choose_language")
        }

        delegate: TextRow {
            text: modelData.language
            onClicked: {
                page.selected(modelData.code, modelData.language)
                pageStack.pop()
            }
        }

        VerticalScrollDecorator {}
    }
}
