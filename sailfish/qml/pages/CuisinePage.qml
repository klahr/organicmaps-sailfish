import QtQuick 2.6
import Sailfish.Silica 1.0

// Cuisines of a place to tick, with search, like the Android cuisine picker.
Dialog {
    id: dialog

    property QtObject placeEditor
    // Keys separated by ';', changed when accepted.
    property string value
    property var selected: value !== "" ? value.split(";") : []

    readonly property var allCuisines: placeEditor.cuisines()
    // The search field is in the list header, out of reach by id.
    property string query

    allowedOrientations: Orientation.All
    onAccepted: value = selected.join(";")

    SilicaListView {
        id: listView
        anchors.fill: parent
        model: dialog.query === "" ? dialog.allCuisines : dialog.allCuisines.filter(function(cuisine) {
            return cuisine.name.toLowerCase().indexOf(dialog.query) >= 0 || cuisine.key.indexOf(dialog.query) >= 0
        })
        currentIndex: -1

        header: Column {
            width: listView.width

            DialogHeader {
                title: appInfo.localized("select_cuisine")
            }
            SearchField {
                width: parent.width
                placeholderText: appInfo.localized("search_in_the_list")
                onTextChanged: dialog.query = text.trim().toLowerCase()
            }
        }

        delegate: TextSwitch {
            text: modelData.name
            automaticCheck: false
            checked: dialog.selected.indexOf(modelData.key) >= 0
            onClicked: {
                var selected = dialog.selected.slice()
                if (checked)
                    selected.splice(selected.indexOf(modelData.key), 1)
                else
                    selected.push(modelData.key)
                dialog.selected = selected
            }
        }

        VerticalScrollDecorator {}
    }
}
