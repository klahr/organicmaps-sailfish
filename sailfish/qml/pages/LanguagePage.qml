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

        delegate: BackgroundItem {
            height: Theme.itemSizeSmall
            onClicked: {
                page.selected(modelData.code, modelData.language)
                pageStack.pop()
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                anchors.verticalCenter: parent.verticalCenter
                text: modelData.language
                color: parent.highlighted ? Theme.highlightColor : Theme.primaryColor
                truncationMode: TruncationMode.Fade
            }
        }

        VerticalScrollDecorator {}
    }
}
