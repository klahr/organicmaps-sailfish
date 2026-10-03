import QtQuick 2.6
import Sailfish.Silica 1.0

// About and help links, like the Android HelpFragment. Texts come from the shared data/strings.
Page {
    id: page

    allowedOrientations: Orientation.All

    readonly property string siteUrl: appInfo.localized("translated_om_site_url")

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: "Organic Maps"
                description: appInfo.version
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

            SectionHeader {
                text: "OpenStreetMap"
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: appInfo.localized("osm_presentation",
                                        [Qt.formatDate(appInfo.dataVersion, Locale.ShortFormat)])
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
            }
            TextRow {
                text: "OpenStreetMap.org"
                url: appInfo.localized("osm_wiki_about_url")
            }

            SectionHeader {
                text: appInfo.localized("help")
            }
            TextRow {
                text: appInfo.localized("news")
                url: page.siteUrl + "news/"
            }
            TextRow {
                text: appInfo.localized("faq")
                url: page.siteUrl + "faq/"
            }
            TextRow {
                text: appInfo.localized("how_to_support_us")
                url: page.siteUrl + "support-us/"
            }
            TextRow {
                text: appInfo.localized("website")
                url: page.siteUrl
            }
            TextRow {
                text: "GitHub"
                url: "https://github.com/organicmaps/organicmaps"
            }
            TextRow {
                text: "Telegram"
                url: appInfo.localized("telegram_url")
            }
            TextRow {
                text: "Matrix"
                url: "https://matrix.to/#/%23organicmaps:matrix.org"
            }
            TextRow {
                text: "Mastodon"
                url: "https://fosstodon.org/@organicmaps"
            }
            TextRow {
                text: appInfo.localized("privacy_policy")
                url: page.siteUrl + "privacy/"
            }
            TextRow {
                text: appInfo.localized("terms_of_use")
                url: page.siteUrl + "terms/"
            }
        }

        VerticalScrollDecorator {}
    }
}
