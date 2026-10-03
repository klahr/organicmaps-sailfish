.import Sailfish.Silica 1.0 as Silica
.import app.organicmaps 1.0 as OM

// Runs a map download action unless the Mobile Internet setting forbids it on a cellular connection, asking
// first when set to, like NetworkPolicy on Android.
function start(pageStack, action) {
    switch (appSettings.downloadPermission()) {
    case OM.AppSettings.DownloadAllowed:
        action()
        break
    case OM.AppSettings.DownloadAsk:
        pageStack.push(Qt.resolvedUrl("MobileDataDialog.qml"), { acceptAction: action })
        break
    default:
        Silica.Notices.show(appInfo.localized("mobile_data_downloads_off"), Silica.Notice.Short,
                            Silica.Notice.Center)
    }
}
