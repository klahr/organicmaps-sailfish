import QtQuick 2.6
import Sailfish.Silica 1.0

// A text field that shows its error message, if any, in place of its title.
TextField {
    property string title
    property string error

    width: parent.width
    label: error !== "" ? error : title
    placeholderText: title
    errorHighlight: error !== ""
    EnterKey.iconSource: "image://theme/icon-m-enter-close"
    EnterKey.onClicked: focus = false
}
