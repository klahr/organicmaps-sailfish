import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Share 1.0
import "clipboard.js" as ClipboardHelper

// About and help links in the order of the Android about.xml. Texts come from the shared data/strings; the
// FAQ and copyright pages are the bundled ones, opened in the browser.
Page {
    id: page

    allowedOrientations: Orientation.All

    readonly property string siteUrl: appInfo.localized("translated_om_site_url")

    function icon(name) {
        return Qt.resolvedUrl("../../icons/help/" + name)
    }
    function bundled(file) {
        return Qt.resolvedUrl("../../data/" + file)
    }

    ShareAction {
        id: logShare
        mimeType: "text/plain"
        resources: [appSettings.logUrl]
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width

            // Tapping copies the app and map data versions, like on iOS, e.g. for bug reports.
            PageHeader {
                title: "Organic Maps"
                description: appInfo.version

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        ClipboardHelper.copy(appInfo.version + " • " + Qt.formatDate(appInfo.dataVersion, "yyMMdd"))
                    }
                }
            }

            Icon {
                anchors.horizontalCenter: parent.horizontalCenter
                width: Theme.iconSizeExtraLarge
                height: width
                sourceSize: Qt.size(width, height)
                source: page.icon("logo.svg")
                color: Theme.highlightColor
            }
            Item {
                width: 1
                height: Theme.paddingLarge
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: appInfo.localized("about_headline")
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeLarge
                wrapMode: Text.Wrap
            }
            Item {
                width: 1
                height: Theme.paddingMedium
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: [appInfo.localized("about_proposition_1"), appInfo.localized("about_proposition_2"),
                       appInfo.localized("about_proposition_3"), "",
                       appInfo.localized("about_developed_by_enthusiasts")].join("\n")
                wrapMode: Text.Wrap
                color: Theme.secondaryHighlightColor
            }

            // The donation box of Android, when the server offers a donation page.
            MenuRow {
                visible: appSettings.donateUrl !== ""
                icon: page.icon("ic_donate.svg")
                text: appInfo.localized("donate")
                onClicked: Qt.openUrlExternally(appSettings.donateUrl)
            }

            SectionHeader {
                text: "OpenStreetMap"
            }
            Row {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                spacing: Theme.paddingLarge

                Image {
                    id: osmLogo
                    width: Theme.iconSizeLarge
                    height: width
                    sourceSize: Qt.size(width, height)
                    source: page.icon("ic_openstreetmap_color.webp")
                }
                Label {
                    width: parent.width - osmLogo.width - parent.spacing
                    anchors.verticalCenter: parent.verticalCenter
                    text: appInfo.localized("osm_presentation",
                                            [Qt.formatDate(appInfo.dataVersion, Locale.ShortFormat)])
                    wrapMode: Text.Wrap
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.secondaryColor
                }
            }
            Item {
                width: 1
                height: Theme.paddingMedium
            }

            Repeater {
                model: [
                    { icon: "ic_question_mark.svg", text: appInfo.localized("faq"), url: page.bundled("faq.html") },
                    { icon: "ic_report_a_bug.svg", text: appInfo.localized("report_a_bug"),
                      url: "https://github.com/organicmaps/organicmaps/issues" },
                    { icon: "ic_donate.svg", text: appInfo.localized("how_to_support_us"), url: page.siteUrl + "support-us/" },
                    { icon: "ic_news.svg", text: appInfo.localized("news"), url: page.siteUrl + "news/" },
                    { icon: "ic_telegram.svg", text: "Telegram", url: appInfo.localized("telegram_url") },
                    { icon: "ic_github.svg", text: "GitHub", url: "https://github.com/organicmaps/organicmaps" },
                    { icon: "ic_website.svg", text: appInfo.localized("website"), url: page.siteUrl },
                    { icon: "ic_matrix.svg", text: "Matrix", url: "https://matrix.to/#/%23organicmaps:matrix.org" },
                    { icon: "ic_mastodon.svg", text: "Mastodon", url: "https://fosstodon.org/@organicmaps" },
                    { icon: "ic_facebook_white.svg", text: "Facebook", url: "https://www.facebook.com/OrganicMaps" },
                    { icon: "ic_twitterx.svg", text: "X (Twitter)", url: "https://twitter.com/OrganicMapsApp" },
                    { icon: "ic_instagram.svg", text: "Instagram", url: appInfo.localized("instagram_url") },
                    { icon: "ic_openstreetmap.svg", text: "OpenStreetMap", url: appInfo.localized("osm_wiki_about_url") },
                    { icon: "ic_openstreetmap.svg", text: appInfo.localized("report_incorrect_map_bug"),
                      url: "https://www.openstreetmap.org/fixthemap" }
                ]

                MenuRow {
                    icon: page.icon(modelData.icon)
                    text: modelData.text
                    // With logging on, a bug report gets the log, as Android attaches it to its bug report mail.
                    onClicked: {
                        if (modelData.icon === "ic_report_a_bug.svg" && appSettings.logging)
                            logShare.trigger()
                        else
                            Qt.openUrlExternally(modelData.url)
                    }
                }
            }

            Item {
                width: 1
                height: Theme.paddingLarge
            }
            TextRow {
                text: appInfo.localized("privacy_policy")
                url: page.siteUrl + "privacy/"
            }
            TextRow {
                text: appInfo.localized("terms_of_use")
                url: page.siteUrl + "terms/"
            }
            TextRow {
                text: appInfo.localized("copyright")
                url: page.bundled("copyright.html")
            }
        }

        VerticalScrollDecorator {}
    }
}
