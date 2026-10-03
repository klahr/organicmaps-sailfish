import QtQuick 2.6
import Sailfish.Silica 1.0

// OpenStreetMap login with password, then the account and its edits, like the Android OsmLoginFragment
// and ProfileFragment.
Page {
    id: page

    allowedOrientations: Orientation.All

    Connections {
        target: osmAccount
        onLoginFailed: errorLabel.text = message
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        PullDownMenu {
            visible: osmAccount.loggedIn

            MenuItem {
                text: appInfo.localized("logout")
                onClicked: Remorse.popupAction(page, appInfo.localized("logout"), function() { osmAccount.logout() })
            }
        }

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: osmAccount.loggedIn ? appInfo.localized("osm_account") : appInfo.localized("login_osm")
            }

            // Logged out.
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: !osmAccount.loggedIn
                text: appInfo.localized("login_osm_presentation")
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.highlightColor
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: !osmAccount.loggedIn && osmAccount.pendingEdits > 0
                text: appInfo.localized("login_to_make_edits_visible")
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
            }
            TextField {
                id: userField
                width: parent.width
                visible: !osmAccount.loggedIn
                enabled: !osmAccount.busy
                label: appInfo.localized("email_or_username")
                placeholderText: label
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: passwordField.focus = true
            }
            PasswordField {
                id: passwordField
                width: parent.width
                visible: !osmAccount.loggedIn
                enabled: !osmAccount.busy
                label: appInfo.localized("password")
                placeholderText: label
                EnterKey.enabled: loginButton.enabled
                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: loginButton.clicked(null)
            }
            Label {
                id: errorLabel
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: !osmAccount.loggedIn && text !== ""
                wrapMode: Text.Wrap
                color: Theme.errorColor
            }
            Item {
                width: parent.width
                height: loginButton.height
                visible: !osmAccount.loggedIn

                Button {
                    id: loginButton
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: !osmAccount.busy
                    enabled: userField.text.trim() !== "" && passwordField.text !== ""
                    text: appInfo.localized("login")
                    onClicked: {
                        errorLabel.text = ""
                        passwordField.focus = false
                        osmAccount.login(userField.text, passwordField.text)
                    }
                }
                BusyIndicator {
                    anchors.centerIn: parent
                    size: BusyIndicatorSize.Medium
                    running: osmAccount.busy
                }
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: !osmAccount.loggedIn
                text: appInfo.localized("forgot_password")
                onClicked: Qt.openUrlExternally(osmAccount.resetPasswordUrl)
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: !osmAccount.loggedIn
                horizontalAlignment: Text.AlignHCenter
                text: appInfo.localized("no_osm_account")
                color: Theme.secondaryHighlightColor
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: !osmAccount.loggedIn
                text: appInfo.localized("register_at_openstreetmap")
                onClicked: Qt.openUrlExternally(osmAccount.registrationUrl)
            }

            // Logged in.
            Column {
                width: parent.width
                visible: osmAccount.loggedIn

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * x
                    text: osmAccount.userName
                    font.pixelSize: Theme.fontSizeLarge
                    color: Theme.highlightColor
                    truncationMode: TruncationMode.Fade
                }
                DetailItem {
                    label: appInfo.localized("editor_profile_changes")
                    value: osmAccount.changesets >= 0 ? osmAccount.changesets : "—"
                }
                DetailItem {
                    label: appInfo.localized("editor_pending_edits")
                    value: osmAccount.pendingEdits
                }
                DetailItem {
                    visible: !isNaN(osmAccount.lastUpload.getTime())
                    label: appInfo.localized("last_upload")
                    value: Format.formatDate(osmAccount.lastUpload, Formatter.DurationElapsed)
                }
                BusyIndicator {
                    anchors.horizontalCenter: parent.horizontalCenter
                    size: BusyIndicatorSize.Small
                    running: osmAccount.busy
                    visible: running
                }
                MenuRow {
                    visible: osmAccount.userName !== ""
                    icon: "image://theme/icon-m-website"
                    text: appInfo.localized("editor_osm_history")
                    onClicked: Qt.openUrlExternally(osmAccount.historyUrl)
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
