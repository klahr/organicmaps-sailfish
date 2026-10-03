import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0

// Edits the selected place, like the Android EditorFragment. Changes are saved locally and uploaded
// to OpenStreetMap once logged in; without an account the login page follows.
Dialog {
    id: dialog

    property string street

    allowedOrientations: Orientation.All
    canAccept: editor.valid && valid()

    function valid() {
        if (nameField.visible && nameField.error !== "")
            return false
        if (addressColumn.visible && houseField.error !== "")
            return false
        for (var i = 0; i < fieldsRepeater.count; ++i) {
            var field = fieldsRepeater.itemAt(i)
            if (field && field.item && field.item.error !== "")
                return false
        }
        return true
    }

    PlaceEditor {
        id: editor
    }

    Component.onCompleted: {
        editor.start()
        street = editor.street
        if (!osmAccount.loggedIn) {
            acceptDestination = Qt.resolvedUrl("OsmAccountPage.qml")
            acceptDestinationAction = PageStackAction.Replace
        }
    }

    onAccepted: {
        if (editor.nameEditable)
            editor.name = nameField.text
        if (editor.addressEditable) {
            editor.street = street
            editor.houseNumber = houseField.text
        }
        for (var i = 0; i < fieldsRepeater.count; ++i) {
            var field = fieldsRepeater.itemAt(i)
            editor.setField(field.fieldId, field.item.value)
        }
        if (editor.save()) {
            osmAccount.updateEdits()
            osmAccount.uploadChanges()
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        ViewPlaceholder {
            enabled: !editor.valid
            text: appInfo.localized("editor_category_unsuitable_title")
        }

        Column {
            id: column
            width: parent.width
            visible: editor.valid

            DialogHeader {
                acceptText: appInfo.localized("save")
                cancelText: appInfo.localized("cancel")
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: editor.category
                font.pixelSize: Theme.fontSizeLarge
                color: Theme.highlightColor
                wrapMode: Text.Wrap
                bottomPadding: Theme.paddingMedium
            }

            TextField {
                id: nameField
                readonly property string error: editor.nameError(text)
                width: parent.width
                visible: editor.nameEditable
                text: editor.name
                label: error !== "" ? error : appInfo.localized("place_name")
                placeholderText: appInfo.localized("editor_default_language_hint")
                errorHighlight: error !== ""
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
            }

            Column {
                id: addressColumn
                width: parent.width
                visible: editor.addressEditable

                ValueButton {
                    label: appInfo.localized("street")
                    value: dialog.street !== "" ? dialog.street : appInfo.localized("choose_street")
                    onClicked: {
                        var page = pageStack.push(Qt.resolvedUrl("StreetPage.qml"),
                                                  { streets: editor.nearbyStreets, current: dialog.street })
                        page.selected.connect(function(street) { dialog.street = street })
                    }
                }
                TextField {
                    id: houseField
                    readonly property string error: editor.houseNumberError(text)
                    width: parent.width
                    text: editor.houseNumber
                    label: error !== "" ? error : appInfo.localized("house_number")
                    placeholderText: appInfo.localized("house_number")
                    errorHighlight: error !== ""
                    inputMethodHints: Qt.ImhNoPredictiveText
                    EnterKey.iconSource: "image://theme/icon-m-enter-close"
                    EnterKey.onClicked: focus = false
                }
            }

            Repeater {
                id: fieldsRepeater
                model: editor.fields

                Loader {
                    readonly property int fieldId: modelData.id
                    // The loaded components see the field through their parent, not modelData.
                    readonly property var field: modelData
                    width: column.width
                    sourceComponent: modelData.kind === PlaceEditor.Wifi ? wifiField
                                   : modelData.kind === PlaceEditor.SelfService ? selfServiceField : textField
                }
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

    // Field delegates; each has the value to save and an error message for an invalid one.
    Component {
        id: textField

        TextField {
            readonly property string value: text
            readonly property string error: editor.fieldError(parent.fieldId, text)
            text: parent.field.value
            label: error !== "" ? error : parent.field.label
            placeholderText: parent.field.label
            errorHighlight: error !== ""
            inputMethodHints: {
                switch (parent.field.inputHint) {
                case "url": return Qt.ImhUrlCharactersOnly | Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
                case "email": return Qt.ImhEmailCharactersOnly
                case "phone": return Qt.ImhPreferNumbers | Qt.ImhNoPredictiveText
                default: return Qt.ImhNone
                }
            }
            EnterKey.iconSource: "image://theme/icon-m-enter-close"
            EnterKey.onClicked: focus = false
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
            readonly property var values: editor.selfServiceValues()
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
