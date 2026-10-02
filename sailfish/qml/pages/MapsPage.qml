import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0

// Map downloader like on Android: it opens on the downloaded maps with a map search, and the pull-down
// menu leads to the maps left to download, grouped by first letter after the ones near the position.
Page {
    id: page

    property alias parentId: countries.parentId
    property alias downloadedOnly: countries.downloadedOnly

    allowedOrientations: Orientation.All

    CountriesModel {
        id: countries
        downloadedOnly: true
    }

    readonly property bool isRoot: countries.parentId === "Countries"
    readonly property bool searching: countries.query !== ""

    function statusText(status, error, progress) {
        switch (status) {
        case CountriesModel.Downloading:
            return qsTr("Downloading %1%").arg(Math.round(progress * 100))
        case CountriesModel.Applying:
            return qsTr("Applying update")
        case CountriesModel.InQueue:
            return qsTr("Waiting to download")
        case CountriesModel.Error:
            return error === 3 ? qsTr("No internet connection")
                               : error === 2 ? qsTr("Not enough space")
                                             : qsTr("Download failed")
        case CountriesModel.OnDiskOutOfDate:
            return qsTr("Update available")
        default:
            return ""
        }
    }

    function showOnMap(countryId) {
        countries.showOnMap(countryId)
        pageStack.pop(pageStack.find(function (p) { return p.objectName === "mapPage" }))
    }

    // Kept outside the list: results reset the model, which would take the focus from a field in its header.
    Column {
        id: header
        width: parent.width

        PageHeader {
            title: !page.isRoot ? countries.title
                                : page.downloadedOnly ? appInfo.localized("download_maps")
                                                      : appInfo.localized("downloader_available_maps")
        }
        SearchField {
            width: parent.width
            visible: page.isRoot
            placeholderText: appInfo.localized("downloader_search_field_hint")
            inputMethodHints: Qt.ImhNoPredictiveText
            onTextChanged: countries.query = text
            EnterKey.iconSource: "image://theme/icon-m-enter-close"
            EnterKey.onClicked: focus = false
        }
    }

    SilicaListView {
        id: list
        anchors {
            top: header.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        clip: true
        model: countries

        // The Android "+" button.
        PullDownMenu {
            visible: page.isRoot && page.downloadedOnly
            MenuItem {
                text: appInfo.localized("download_maps")
                onClicked: pageStack.push(Qt.resolvedUrl("MapsPage.qml"), { downloadedOnly: false })
            }
        }

        section.property: "section"
        section.delegate: SectionHeader {
            text: section
        }

        delegate: ListItem {
            id: item

            readonly property bool busy: model.status === CountriesModel.Downloading
                                         || model.status === CountriesModel.InQueue
                                         || model.status === CountriesModel.Applying
            readonly property bool hasLocal: model.status === CountriesModel.OnDisk
                                             || model.status === CountriesModel.OnDiskOutOfDate
                                             || model.status === CountriesModel.Partly
            readonly property string status: page.statusText(model.status, model.error, model.progress)

            contentHeight: Theme.itemSizeMedium
            menu: contextMenu

            onClicked: {
                if (model.isGroup)
                    pageStack.push(Qt.resolvedUrl("MapsPage.qml"),
                                   { parentId: model.countryId, downloadedOnly: page.downloadedOnly && !page.searching })
                else if (model.status === CountriesModel.NotDownloaded)
                    countries.download(model.countryId)
                else if (model.status === CountriesModel.Error)
                    countries.retry(model.countryId)
                else if (model.status === CountriesModel.OnDisk)
                    page.showOnMap(model.countryId)
                else
                    openMenu()
            }

            // Round status icon at the start of the row, like on Android.
            Rectangle {
                id: statusIcon
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                width: Theme.iconSizeMedium + Theme.paddingSmall
                height: width
                radius: width / 2
                color: item.hasLocal && !model.isGroup ? Theme.rgba(Theme.primaryColor, 0.1)
                                                       : Theme.rgba(Theme.highlightBackgroundColor, 0.5)

                Icon {
                    anchors.centerIn: parent
                    visible: !item.busy
                    sourceSize: Qt.size(Theme.iconSizeSmallPlus, Theme.iconSizeSmallPlus)
                    source: model.isGroup ? "image://theme/icon-m-file-folder"
                          : model.status === CountriesModel.OnDiskOutOfDate ? "image://theme/icon-m-refresh"
                          : model.status === CountriesModel.Error ? "image://theme/icon-m-reload"
                          : item.hasLocal ? "image://theme/icon-m-acknowledge"
                          : "../../icons/menu/ic_download.svg"
                }
                ProgressCircle {
                    anchors.fill: parent
                    visible: item.busy
                    value: model.progress
                    progressColor: Theme.highlightColor
                    backgroundColor: Theme.rgba(Theme.highlightDimmerColor, 0.5)
                }
            }

            Column {
                anchors {
                    left: statusIcon.right
                    leftMargin: Theme.paddingLarge
                    right: sizeLabel.left
                    rightMargin: Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }

                Label {
                    width: parent.width
                    text: page.searching && model.foundName !== "" ? model.foundName : model.name
                    truncationMode: TruncationMode.Fade
                    color: item.highlighted ? Theme.highlightColor : Theme.primaryColor
                }
                Label {
                    width: parent.width
                    // Status while it matters, else the group count or the main cities, as on Android.
                    text: item.status !== "" ? item.status
                        : model.isGroup ? appInfo.localized("downloader_status_maps") + ": "
                                          + appInfo.localized("downloader_of", [model.localMapsCount, model.mapsCount])
                        : page.searching ? model.parentName
                        : model.description
                    visible: text !== ""
                    font.pixelSize: Theme.fontSizeExtraSmall
                    truncationMode: TruncationMode.Fade
                    color: model.status === CountriesModel.Error ? Theme.errorColor
                                                                 : item.highlighted ? Theme.secondaryHighlightColor
                                                                                    : Theme.secondaryColor
                }
            }

            Label {
                id: sizeLabel
                anchors {
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                // The downloaded size in the downloaded list, else the full size.
                text: countries.formatSize(page.downloadedOnly && !page.searching && !item.busy ? model.localSize
                                                                                                : model.size)
                font.pixelSize: Theme.fontSizeSmall
                color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
            }

            Component {
                id: contextMenu
                ContextMenu {
                    MenuItem {
                        text: qsTr("Download")
                        visible: model.status === CountriesModel.NotDownloaded || model.status === CountriesModel.Partly
                        onClicked: countries.download(model.countryId)
                    }
                    MenuItem {
                        text: qsTr("Retry")
                        visible: model.status === CountriesModel.Error
                        onClicked: countries.retry(model.countryId)
                    }
                    MenuItem {
                        text: qsTr("Update")
                        visible: model.status === CountriesModel.OnDiskOutOfDate
                        onClicked: countries.update(model.countryId)
                    }
                    MenuItem {
                        text: qsTr("Cancel download")
                        visible: item.busy
                        onClicked: countries.cancel(model.countryId)
                    }
                    MenuItem {
                        text: qsTr("Show on map")
                        visible: item.hasLocal
                        onClicked: page.showOnMap(model.countryId)
                    }
                    MenuItem {
                        text: qsTr("Delete")
                        visible: item.hasLocal
                        onClicked: {
                            var countryId = model.countryId
                            item.remorseDelete(function () { countries.remove(countryId) })
                        }
                    }
                }
            }
        }

        ViewPlaceholder {
            enabled: list.count === 0 && page.downloadedOnly && !page.searching
            text: qsTr("No maps downloaded yet")
            hintText: qsTr("Pull down to download maps")
        }

        VerticalScrollDecorator {}
    }
}
