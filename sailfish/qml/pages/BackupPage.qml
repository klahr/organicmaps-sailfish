import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0

// Backups of all bookmarks and tracks as KMZ files in a folder, restored with Import on the bookmarks page.
Page {
    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: appInfo.localized("backup")
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: appInfo.localized("backup_description")
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
                bottomPadding: Theme.paddingLarge
            }
            ComboBox {
                // Days between backups.
                readonly property var periods: [0, 1, 7]
                label: appInfo.localized("backup_automatic")
                currentIndex: Math.max(0, periods.indexOf(bookmarksIO.backupPeriod))
                menu: ContextMenu {
                    MenuItem { text: appInfo.localized("off") }
                    MenuItem { text: appInfo.localized("backup_daily") }
                    MenuItem { text: appInfo.localized("backup_weekly") }
                }
                onCurrentIndexChanged: bookmarksIO.backupPeriod = periods[currentIndex]
            }
            ValueButton {
                label: appInfo.localized("backup_folder")
                value: bookmarksIO.backupFolder
                valueColor: Theme.secondaryHighlightColor
                onClicked: pageStack.push(folderPicker)
            }
            Item {
                width: 1
                height: Theme.paddingLarge
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: appInfo.localized("backup_now")
                enabled: !bookmarksIO.backingUp
                onClicked: bookmarksIO.backUpNow()
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                topPadding: Theme.paddingLarge
                horizontalAlignment: Text.AlignHCenter
                text: isNaN(bookmarksIO.lastBackup.getTime())
                      ? appInfo.localized("backup_none")
                      : appInfo.localized("backup_last", [Format.formatDate(bookmarksIO.lastBackup,
                                                                            Formatter.DateMedium)])
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
            }
        }

        VerticalScrollDecorator {}
    }

    Component {
        id: folderPicker

        FolderPickerPage {
            dialogTitle: appInfo.localized("backup_folder")
            onSelectedPathChanged: bookmarksIO.backupFolder = selectedPath
        }
    }
}
