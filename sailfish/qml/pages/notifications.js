.pragma library

// D-Bus actions of the app's notifications, handled by UrlHandler.
function action(name, displayName, method, args) {
    return {
        "name": name,
        "displayName": displayName,
        "service": "app.organicmaps.organicmaps",
        "path": "/app/organicmaps",
        "iface": "app.organicmaps.organicmaps",
        "method": method,
        "arguments": args || []
    }
}

// Tapping the notification brings the app to the front.
function openApp() {
    return action("default", "", "openUrl", [[]])
}
