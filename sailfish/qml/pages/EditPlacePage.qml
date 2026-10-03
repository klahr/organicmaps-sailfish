import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0

// Edits the selected place or creates a new one, laid out like the Android EditorFragment: category, names,
// address, details, social media and a note. Changes are saved locally and uploaded to OpenStreetMap once
// logged in; without an account the login page follows.
Dialog {
    id: dialog

    property string street
    // A new place of this type at lat, lon, from the category page; otherwise the selected place.
    property string newPlaceType
    property real lat
    property real lon
    // Names in other languages as {code, language, value}, including the ones added here.
    property var names: []

    allowedOrientations: Orientation.All
    canAccept: editor.valid && valid()

    readonly property var fieldRepeaters: [addressFields, detailFields, socialFields]

    function valid() {
        if (nameField.visible && nameField.error !== "")
            return false
        for (var n = 0; n < namesRepeater.count; ++n)
            if (namesRepeater.itemAt(n).error !== "")
                return false
        if (addressColumn.visible && houseField.error !== "")
            return false
        for (var r = 0; r < fieldRepeaters.length; ++r) {
            for (var i = 0; i < fieldRepeaters[r].count; ++i) {
                var field = fieldRepeaters[r].itemAt(i)
                if (field && field.item && field.item.error !== "")
                    return false
            }
        }
        return true
    }

    function fieldsOf(section) {
        return editor.fields.filter(function(field) { return field.section === section })
    }

    PlaceEditor {
        id: editor
    }
    // For the field delegates: TextField has an "editor" property of its own that hides the id.
    readonly property alias placeEditor: editor

    Component.onCompleted: {
        if (newPlaceType !== "")
            editor.create(newPlaceType, lat, lon)
        else
            editor.start()
        street = editor.street
        names = editor.localizedNames
        if (!osmAccount.loggedIn) {
            acceptDestination = Qt.resolvedUrl("OsmAccountPage.qml")
            acceptDestinationAction = PageStackAction.Replace
        }
    }

    onAccepted: {
        if (editor.nameEditable) {
            editor.name = nameField.text
            for (var n = 0; n < namesRepeater.count; ++n)
                editor.setLocalizedName(names[n].code, namesRepeater.itemAt(n).text)
        }
        if (editor.addressEditable) {
            editor.street = street
            editor.houseNumber = houseField.text
        }
        for (var r = 0; r < fieldRepeaters.length; ++r) {
            for (var i = 0; i < fieldRepeaters[r].count; ++i) {
                var field = fieldRepeaters[r].itemAt(i)
                editor.setField(field.fieldId, field.item.value)
            }
        }
        if (editor.save()) {
            editor.createNote(noteField.text)
            osmAccount.updateEdits()
            osmAccount.uploadChanges()
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        ViewPlaceholder {
            enabled: !editor.valid
            text: dialog.newPlaceType !== "" ? appInfo.localized("message_invalid_feature_position")
                                             : appInfo.localized("editor_category_unsuitable_title")
        }

        Column {
            id: column
            width: parent.width
            visible: editor.valid

            DialogHeader {
                title: dialog.newPlaceType !== "" ? appInfo.localized("editor_add_place_title")
                                                  : appInfo.localized("editor_edit_place_title")
                acceptText: appInfo.localized("save")
                cancelText: appInfo.localized("cancel")
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: appInfo.localized("editor_about_osm")
                textFormat: Text.StyledText
                linkColor: Theme.highlightColor
                onLinkActivated: Qt.openUrlExternally(link)
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                bottomPadding: Theme.paddingLarge
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: appInfo.localized("editor_edit_place_category_title")
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: editor.category
                color: Theme.highlightColor
                wrapMode: Text.Wrap
                bottomPadding: Theme.paddingMedium
            }

            Column {
                width: parent.width
                visible: editor.nameEditable

                SectionHeader {
                    text: appInfo.localized("editor_edit_place_name_hint")
                }
                ValidatedTextField {
                    id: nameField
                    text: dialog.placeEditor.name
                    title: appInfo.localized("place_name")
                    error: dialog.placeEditor.nameError(text)
                    placeholderText: appInfo.localized("editor_default_language_hint")
                }
                Repeater {
                    id: namesRepeater
                    model: dialog.names

                    ValidatedTextField {
                        text: modelData.value
                        title: modelData.language
                        error: dialog.placeEditor.nameError(text)
                    }
                }
                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: appInfo.localized("add_language")
                    onClicked: {
                        var used = dialog.names.map(function(name) { return name.code })
                        var page = pageStack.push(Qt.resolvedUrl("LanguagePage.qml"), {
                            languages: editor.otherLanguages().filter(function(language) {
                                return used.indexOf(language.code) < 0
                            })
                        })
                        page.selected.connect(function(code, language) {
                            // Keeps the typed names: the Repeater recreates its fields for the new model.
                            var names = []
                            for (var n = 0; n < namesRepeater.count; ++n)
                                names.push({ code: dialog.names[n].code, language: dialog.names[n].language,
                                             value: namesRepeater.itemAt(n).text })
                            names.push({ code: code, language: language, value: "" })
                            dialog.names = names
                        })
                    }
                }
            }

            Column {
                id: addressColumn
                width: parent.width
                visible: editor.addressEditable

                SectionHeader {
                    text: appInfo.localized("address")
                }
                EditorRow {
                    icon: "../../icons/editor/ic_street_address.svg"

                    ValueButton {
                        width: parent.width
                        label: appInfo.localized("street")
                        value: dialog.street !== "" ? dialog.street : appInfo.localized("choose_street")
                        onClicked: {
                            var page = pageStack.push(Qt.resolvedUrl("StreetPage.qml"),
                                                      { streets: editor.nearbyStreets, current: dialog.street })
                            page.selected.connect(function(street) { dialog.street = street })
                        }
                    }
                }
                EditorRow {
                    icon: "../../icons/editor/ic_building.svg"

                    ValidatedTextField {
                        id: houseField
                        text: dialog.placeEditor.houseNumber
                        title: appInfo.localized("house_number")
                        error: dialog.placeEditor.houseNumberError(text)
                        inputMethodHints: Qt.ImhNoPredictiveText
                    }
                }
                Repeater {
                    id: addressFields
                    model: dialog.fieldsOf(PlaceEditor.Address)
                    delegate: fieldDelegate
                }
            }

            SectionHeader {
                visible: detailFields.count > 0
                text: appInfo.localized("details")
            }
            Repeater {
                id: detailFields
                model: dialog.fieldsOf(PlaceEditor.Details)
                delegate: fieldDelegate
            }

            SectionHeader {
                visible: socialFields.count > 0
                text: appInfo.localized("social_media")
            }
            Repeater {
                id: socialFields
                model: dialog.fieldsOf(PlaceEditor.SocialMedia)
                delegate: fieldDelegate
            }

            SectionHeader {
                text: appInfo.localized("editor_other_info")
            }
            TextArea {
                id: noteField
                width: parent.width
                placeholderText: appInfo.localized("editor_note_hint")
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: editor.canReset
                text: appInfo.localized("editor_reset_edits_button")
                onClicked: Remorse.popupAction(dialog, appInfo.localized("editor_reset_edits_button"), function() {
                    editor.reset()
                    osmAccount.updateEdits()
                    pageStack.pop()
                })
            }
        }

        VerticalScrollDecorator {}
    }

    // A field with its icon, loading the editor for its kind.
    Component {
        id: fieldDelegate

        EditorRow {
            readonly property int fieldId: modelData.id
            readonly property alias item: loader.item
            icon: modelData.icon

            Loader {
                id: loader
                // The loaded components see the field through their parent, not modelData.
                readonly property var field: modelData
                width: parent.width
                sourceComponent: modelData.kind === PlaceEditor.Wifi ? wifiField
                               : modelData.kind === PlaceEditor.SelfService ? selfServiceField : textField
            }
        }
    }

    // Field editors; each has the value to save and an error message for an invalid one.
    Component {
        id: textField

        ValidatedTextField {
            readonly property string value: text
            text: parent.field.value
            title: parent.field.label
            error: dialog.placeEditor.fieldError(parent.field.id, text)
            inputMethodHints: {
                switch (parent.field.inputHint) {
                case "url": return Qt.ImhUrlCharactersOnly | Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
                case "email": return Qt.ImhEmailCharactersOnly
                case "phone": return Qt.ImhPreferNumbers | Qt.ImhNoPredictiveText
                default: return Qt.ImhNone
                }
            }
        }
    }
    Component {
        id: wifiField

        TextSwitch {
            readonly property string value: checked ? "yes" : ""
            readonly property string error: ""
            text: parent.field.label
            checked: parent.field.value === "yes"
        }
    }
    Component {
        id: selfServiceField

        ComboBox {
            readonly property var values: dialog.placeEditor.selfServiceValues()
            readonly property string value: currentIndex > 0 ? values[currentIndex - 1].value : ""
            readonly property string error: ""
            label: parent.field.label
            currentIndex: {
                for (var i = 0; i < values.length; ++i)
                    if (values[i].value === parent.field.value)
                        return i + 1
                return 0
            }
            menu: ContextMenu {
                MenuItem { text: "—" }
                Repeater {
                    model: values
                    MenuItem { text: modelData.name }
                }
            }
        }
    }
}
