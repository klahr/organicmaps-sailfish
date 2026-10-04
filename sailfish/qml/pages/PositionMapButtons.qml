import QtQuick 2.6
import Sailfish.Silica 1.0
import "downloads.js" as Downloads

// Without downloaded maps: the map of the position, then the list of all maps, like the Android "no maps"
// screen.
Column {
    // The map of the position, see MissingMapInfo().
    property var map
    signal downloadClicked(string countryId)

    readonly property bool busy: Downloads.busy(map.status)

    spacing: Theme.paddingLarge

    Button {
        anchors.horizontalCenter: parent.horizontalCenter
        visible: !!map.countryId
        enabled: !parent.busy
        text: parent.busy ? appInfo.localized("downloader_downloading") + " " + Math.round((map.progress || 0) * 100) + "%"
                          : appInfo.localized("downloader_download_map") + " (" + map.size + ")"
        onClicked: parent.downloadClicked(map.countryId)
    }
    Button {
        anchors.horizontalCenter: parent.horizontalCenter
        text: appInfo.localized("search_select_map")
        onClicked: pageStack.push(Qt.resolvedUrl("MapsPage.qml"), { downloadedOnly: false })
    }
}
