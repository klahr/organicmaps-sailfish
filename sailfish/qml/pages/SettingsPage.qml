import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0

// The Android settings that apply to this port, in their Android groups.
Page {
    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: appInfo.localized("settings")
            }

            SectionHeader {
                text: appInfo.localized("prefs_group_general")
            }
            ComboBox {
                label: appInfo.localized("pref_appearance_title")
                currentIndex: appSettings.mapAppearance
                menu: ContextMenu {
                    MenuItem { text: appInfo.localized("follow_system") }
                    MenuItem { text: appInfo.localized("pref_appearance_light") }
                    MenuItem { text: appInfo.localized("pref_appearance_dark") }
                }
                onCurrentIndexChanged: appSettings.mapAppearance = currentIndex
            }
            ComboBox {
                label: appInfo.localized("measurement_units")
                description: appInfo.localized("measurement_units_summary")
                currentIndex: appSettings.units
                menu: ContextMenu {
                    MenuItem { text: appInfo.localized("kilometres") }
                    MenuItem { text: appInfo.localized("miles") }
                }
                onCurrentIndexChanged: appSettings.units = currentIndex
            }
            TextSwitch {
                text: appInfo.localized("pref_zoom_title")
                description: appInfo.localized("pref_zoom_summary")
                checked: appSettings.zoomButtons
                onCheckedChanged: appSettings.zoomButtons = checked
            }
            TextSwitch {
                text: appInfo.localized("pref_map_3d_buildings_title")
                checked: appSettings.buildings3d
                onCheckedChanged: appSettings.buildings3d = checked
            }
            TextSwitch {
                text: appInfo.localized("autodownload")
                checked: appSettings.autoDownload
                onCheckedChanged: appSettings.autoDownload = checked
            }
            TextSwitch {
                text: appInfo.localized("show_downloaded_regions")
                checked: appSettings.showDownloadedRegions
                onCheckedChanged: appSettings.showDownloadedRegions = checked
            }
            TextSwitch {
                text: appInfo.localized("big_font")
                checked: appSettings.largeFonts
                onCheckedChanged: appSettings.largeFonts = checked
            }
            TextSwitch {
                text: appInfo.localized("transliteration_title")
                checked: appSettings.transliteration
                onCheckedChanged: appSettings.transliteration = checked
            }
            ComboBox {
                // Android values of power_management_scheme_values.
                readonly property var schemes: [1, 3, 4]
                label: appInfo.localized("power_managment_title")
                description: appInfo.localized("power_managment_description")
                currentIndex: Math.max(0, schemes.indexOf(appSettings.powerScheme))
                menu: ContextMenu {
                    MenuItem { text: appInfo.localized("power_managment_setting_never") }
                    MenuItem { text: appInfo.localized("power_managment_setting_manual_max") }
                    MenuItem { text: appInfo.localized("power_managment_setting_auto") }
                }
                onCurrentIndexChanged: appSettings.powerScheme = schemes[currentIndex]
            }
            ComboBox {
                label: appInfo.localized("bookmarks_text_placement_title")
                description: appInfo.localized("bookmarks_text_placement_description")
                currentIndex: appSettings.bookmarksTextPlacement
                menu: ContextMenu {
                    MenuItem { text: appInfo.localized("hide") }
                    MenuItem { text: appInfo.localized("show_to_the_right") }
                    MenuItem { text: appInfo.localized("show_at_the_bottom") }
                }
                onCurrentIndexChanged: appSettings.bookmarksTextPlacement = currentIndex
            }
            TextSwitch {
                text: appInfo.localized("enable_keep_screen_on")
                description: appInfo.localized("enable_keep_screen_on_description")
                checked: appSettings.keepScreenOn
                onCheckedChanged: appSettings.keepScreenOn = checked
            }
            ValueButton {
                label: appInfo.localized("change_map_locale")
                value: appSettings.mapLanguageName
                onClicked: pageStack.push(Qt.resolvedUrl("MapLanguagePage.qml"))
            }
            ValueButton {
                label: appInfo.localized("pref_bg_tiles_title")
                value: appSettings.bgTilesEnabled ? appInfo.localized("on") : appInfo.localized("off")
                onClicked: pageStack.push(Qt.resolvedUrl("SatelliteSettingsPage.qml"))
            }

            SectionHeader {
                text: appInfo.localized("privacy")
            }
            TextSwitch {
                text: appInfo.localized("search_history_title")
                checked: appSettings.searchHistory
                onCheckedChanged: appSettings.searchHistory = checked
            }

            SectionHeader {
                text: appInfo.localized("help")
            }
            BackgroundItem {
                width: parent.width
                height: Theme.itemSizeSmall
                onClicked: pageStack.push(Qt.resolvedUrl("HelpPage.qml"))

                Label {
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    text: appInfo.localized("help")
                    color: parent.highlighted ? Theme.highlightColor : Theme.primaryColor
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
