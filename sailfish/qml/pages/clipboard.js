// Copies text and says so, like the Android copy toasts. Not a library: it uses the Silica globals of the page.
function copy(text) {
    Clipboard.text = text
    Notices.show(appInfo.localized("copied_to_clipboard", [text]), Notice.Short, Notice.Center)
}
