import QtQuick 2.6
import Sailfish.Silica 1.0

// Where maps are kept, like the Android "Save maps to" screen. Moving them closes the app afterwards, which
// opens with the new folder.
Page {
    id: page

    allowedOrientations: Orientation.All
    backNavigation: !mapsStorage.moving
    onStatusChanged: if (status === PageStatus.Activating) mapsStorage.refresh()

    property bool moved

    Connections {
        target: mapsStorage
        onMoveFinished: {
            if (success)
                page.moved = true
            else
                Notices.show(appInfo.localized("maps_storage_move_failed"), Notice.Long, Notice.Center)
        }
    }

    SilicaListView {
        id: listView
        anchors.fill: parent
        visible: !mapsStorage.moving && !page.moved
        model: mapsStorage.locations

        header: Column {
            width: listView.width

            PageHeader {
                title: appInfo.localized("maps_storage")
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: appInfo.localized("maps_storage_summary")
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
                bottomPadding: Theme.paddingLarge
            }
            DetailItem {
                label: appInfo.localized("maps_storage_downloaded")
                value: mapsStorage.downloadedSize
            }
        }

        delegate: ListItem {
            id: item
            contentHeight: Theme.itemSizeMedium
            highlighted: down || modelData.current
            onClicked: {
                if (modelData.current)
                    return
                // Like Android: not while a map downloads.
                if (mapsStorage.downloading) {
                    Notices.show(appInfo.localized("cant_change_this_setting") + " "
                                 + appInfo.localized("downloading_is_active"), Notice.Long, Notice.Center)
                    return
                }
                remorseAction(appInfo.localized("move_maps"), function() { mapsStorage.moveTo(modelData.path) })
            }

            Column {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                anchors.verticalCenter: parent.verticalCenter

                Label {
                    width: parent.width
                    text: modelData.name
                    highlighted: item.highlighted
                    truncationMode: TruncationMode.Fade
                }
                Label {
                    width: parent.width
                    text: modelData.details
                    font.pixelSize: Theme.fontSizeSmall
                    color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                }
            }
        }

        VerticalScrollDecorator {}
    }

    BusyLabel {
        running: mapsStorage.moving
        text: appInfo.localized("maps_storage_moving")
    }

    // The maps are in the new folder; the app needs to start again to use them.
    Column {
        anchors.centerIn: parent
        width: parent.width - 2 * Theme.horizontalPageMargin
        visible: page.moved
        spacing: Theme.paddingLarge

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: appInfo.localized("maps_storage_restart")
            wrapMode: Text.Wrap
            color: Theme.highlightColor
            font.pixelSize: Theme.fontSizeLarge
        }
        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            text: appInfo.localized("close")
            onClicked: Qt.quit()
        }
    }
}
