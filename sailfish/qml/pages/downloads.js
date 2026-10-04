.import Sailfish.Silica 1.0 as Silica
.import app.organicmaps 1.0 as OM

// A map is on its way: downloading, queued or being applied.
function busy(status) {
    return status === OM.CountriesModel.Downloading || status === OM.CountriesModel.InQueue
           || status === OM.CountriesModel.Applying
}

// Cancels a map on its way, or downloads it, for a MissingMapInfo() and the MapItem.
function toggle(pageStack, map, country) {
    var countryId = country.countryId
    if (busy(country.status))
        map.cancelMap(countryId)
    else
        start(pageStack, function() { map.downloadMap(countryId) })
}

// Runs a map download action unless the Mobile Internet setting forbids it on a cellular connection, asking
// first when set to, like NetworkPolicy on Android.
function start(pageStack, action) {
    switch (appSettings.downloadPermission()) {
    case OM.AppSettings.DownloadAllowed:
        action()
        break
    case OM.AppSettings.DownloadAsk:
        pageStack.push(Qt.resolvedUrl("MessageDialog.qml"), {
            title: appInfo.localized("download_over_mobile_header"),
            message: appInfo.localized("download_over_mobile_message"),
            acceptText: appInfo.localized("download"),
            acceptAction: action
        })
        break
    default:
        Silica.Notices.show(appInfo.localized("mobile_data_downloads_off"), Silica.Notice.Short,
                            Silica.Notice.Center)
    }
}
