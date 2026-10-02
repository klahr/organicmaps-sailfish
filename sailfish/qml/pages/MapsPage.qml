import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0

Page {
    id: page

    property alias parentId: countries.parentId

    CountriesModel {
        id: countries
    }

    function statusText(status, error, size, localSize, progress, isGroup, mapsCount, localMapsCount) {
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
        case CountriesModel.OnDisk:
            return isGroup ? qsTr("%1 maps downloaded · %2").arg(localMapsCount).arg(countries.formatSize(localSize))
                           : qsTr("Downloaded · %1").arg(countries.formatSize(localSize))
        case CountriesModel.Partly:
            return qsTr("%1 of %2 maps downloaded").arg(localMapsCount).arg(mapsCount)
        default:
            return isGroup ? qsTr("%1 maps · %2").arg(mapsCount).arg(countries.formatSize(size))
                           : countries.formatSize(size)
        }
    }

    function showOnMap(countryId) {
        countries.showOnMap(countryId)
        pageStack.pop(pageStack.find(function (p) { return p.objectName === "mapPage" }))
    }

    SilicaListView {
        id: list
        anchors.fill: parent
        model: countries

        header: PageHeader {
            title: countries.parentId === "Countries" ? qsTr("Download maps") : countries.title
        }

        delegate: ListItem {
            id: item

            readonly property bool busy: model.status === CountriesModel.Downloading
                                         || model.status === CountriesModel.InQueue
                                         || model.status === CountriesModel.Applying
            readonly property bool hasLocal: model.status === CountriesModel.OnDisk
                                             || model.status === CountriesModel.OnDiskOutOfDate
                                             || model.status === CountriesModel.Partly

            contentHeight: Theme.itemSizeMedium
            menu: contextMenu

            onClicked: {
                if (model.isGroup)
                    pageStack.push(Qt.resolvedUrl("MapsPage.qml"), { parentId: model.countryId })
                else if (model.status === CountriesModel.NotDownloaded)
                    countries.download(model.countryId)
                else if (model.status === CountriesModel.Error)
                    countries.retry(model.countryId)
                else if (model.status === CountriesModel.OnDisk)
                    page.showOnMap(model.countryId)
                else
                    openMenu()
            }

            Column {
                anchors {
                    left: parent.left
                    right: icon.left
                    leftMargin: Theme.horizontalPageMargin
                    rightMargin: Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }

                Label {
                    width: parent.width
                    text: model.name
                    truncationMode: TruncationMode.Fade
                    color: item.highlighted ? Theme.highlightColor : Theme.primaryColor
                }
                Label {
                    width: parent.width
                    text: page.statusText(model.status, model.error, model.size, model.localSize, model.progress,
                                          model.isGroup, model.mapsCount, model.localMapsCount)
                    font.pixelSize: Theme.fontSizeExtraSmall
                    truncationMode: TruncationMode.Fade
                    color: model.status === CountriesModel.Error ? Theme.errorColor
                                                                 : item.highlighted ? Theme.secondaryHighlightColor
                                                                                    : Theme.secondaryColor
                }
            }

            Icon {
                id: icon
                anchors {
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                source: model.isGroup ? "image://theme/icon-m-right"
                                      : item.busy ? "image://theme/icon-m-clear"
                                                  : model.status === CountriesModel.NotDownloaded
                                                    ? "image://theme/icon-m-cloud-download"
                                                    : model.status === CountriesModel.OnDiskOutOfDate
                                                      ? "image://theme/icon-m-refresh"
                                                      : model.status === CountriesModel.Error
                                                        ? "image://theme/icon-m-reload" : ""
            }

            Rectangle {
                anchors.bottom: parent.bottom
                height: Theme.paddingSmall / 2
                width: parent.width * model.progress
                visible: item.busy
                color: Theme.highlightColor
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

        VerticalScrollDecorator {}
    }
}
