import QtQuick 2.6
import Sailfish.Silica 1.0

// Until the first edit or note: they go to everyone, like Android's one-time dialog before the first save.
Column {
    width: parent.width
    visible: !appSettings.editsPublicNoticeShown
    bottomPadding: Theme.paddingMedium

    Label {
        x: Theme.horizontalPageMargin
        width: parent.width - 2 * x
        text: appInfo.localized("editor_share_to_all_dialog_title")
        wrapMode: Text.Wrap
        color: Theme.highlightColor
    }
    Label {
        x: Theme.horizontalPageMargin
        width: parent.width - 2 * x
        text: appInfo.localized("editor_share_to_all_dialog_message_1") + " "
              + appInfo.localized("editor_share_to_all_dialog_message_2")
        wrapMode: Text.Wrap
        font.pixelSize: Theme.fontSizeSmall
        color: Theme.secondaryHighlightColor
    }
}
