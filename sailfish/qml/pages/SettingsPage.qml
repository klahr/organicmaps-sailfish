import QtQuick 2.6
import Sailfish.Silica 1.0

// The Android settings that apply to this port, in their Android groups.
Page {
    // MapItem.routing, for the routing options.
    property QtObject mapPageRouting

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
            // First and without a group, as on Android.
            ValueButton {
                label: appInfo.localized("profile")
                value: osmAccount.loggedIn ? osmAccount.userName : appInfo.localized("not_signed_in")
                onClicked: pageStack.push(Qt.resolvedUrl("OsmAccountPage.qml"))
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
                    MenuItem { text: appInfo.localized("pref_appearance_scheduled") }
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
            // Off under maximum power saving (scheme 3), like on Android.
            TextSwitch {
                readonly property bool powerSaving: appSettings.powerScheme === 3
                text: appInfo.localized("pref_map_3d_buildings_title")
                description: powerSaving ? appInfo.localized("pref_map_3d_buildings_disabled_summary") : ""
                enabled: !powerSaving
                automaticCheck: false
                checked: !powerSaving && appSettings.buildings3d
                onClicked: appSettings.buildings3d = !appSettings.buildings3d
            }
            TextSwitch {
                text: appInfo.localized("autodownload")
                checked: appSettings.autoDownload
                onCheckedChanged: appSettings.autoDownload = checked
            }
            ComboBox {
                label: appInfo.localized("mobile_data")
                description: appInfo.localized("mobile_data_description")
                currentIndex: appSettings.mobileData
                menu: ContextMenu {
                    MenuItem { text: appInfo.localized("mobile_data_option_ask") }
                    MenuItem { text: appInfo.localized("mobile_data_option_always") }
                    MenuItem { text: appInfo.localized("mobile_data_option_never") }
                }
                onCurrentIndexChanged: appSettings.mobileData = currentIndex
            }
            ValueButton {
                label: appInfo.localized("maps_storage")
                value: mapsStorage.currentName
                onClicked: pageStack.push(Qt.resolvedUrl("StoragePage.qml"))
            }
            ValueButton {
                label: appInfo.localized("backup")
                value: bookmarksIO.backupPeriod === 0 ? appInfo.localized("off")
                     : appInfo.localized(bookmarksIO.backupPeriod === 1 ? "backup_daily" : "backup_weekly")
                onClicked: pageStack.push(Qt.resolvedUrl("BackupPage.qml"))
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
                text: appInfo.localized("prefs_group_route")
            }
            TextSwitch {
                text: appInfo.localized("pref_auto_night_in_navigation_title")
                checked: appSettings.autoNightInNavigation
                onCheckedChanged: appSettings.autoNightInNavigation = checked
            }
            TextSwitch {
                text: appInfo.localized("pref_map_3d_title")
                checked: appSettings.perspectiveView
                onCheckedChanged: appSettings.perspectiveView = checked
            }
            ValueButton {
                label: appInfo.localized("pref_tts_enable_title")
                value: !mapPageRouting.voiceAvailable ? appInfo.localized("pref_tts_unavailable")
                     : mapPageRouting.voiceEnabled ? mapPageRouting.voiceLanguageName
                     : appInfo.localized("off")
                onClicked: pageStack.push(Qt.resolvedUrl("VoicePage.qml"), { routing: mapPageRouting })
            }
            TextSwitch {
                text: appInfo.localized("pref_map_auto_zoom")
                checked: appSettings.autoZoom
                onCheckedChanged: appSettings.autoZoom = checked
            }
            ComboBox {
                label: appInfo.localized("speedcams_alert_title")
                currentIndex: appSettings.speedCamerasMode
                menu: ContextMenu {
                    MenuItem { text: appInfo.localized("pref_tts_speedcams_auto") }
                    MenuItem { text: appInfo.localized("pref_tts_speedcams_always") }
                    MenuItem { text: appInfo.localized("pref_tts_speedcams_never") }
                }
                onCurrentIndexChanged: appSettings.speedCamerasMode = currentIndex
            }
            ValueButton {
                label: appInfo.localized("driving_options_title")
                onClicked: pageStack.push(Qt.resolvedUrl("RoutingOptionsPage.qml"), { routing: mapPageRouting })
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
            TextRow {
                text: appInfo.localized("help")
                onClicked: pageStack.push(Qt.resolvedUrl("HelpPage.qml"))
            }
            TextSwitch {
                text: appInfo.localized("enable_logging")
                description: appInfo.localized("enable_logging_warning_message")
                             + (checked ? "\n" + appInfo.localized("log_file_size",
                                                                     [appInfo.formatSize(appSettings.logSize())]) : "")
                checked: appSettings.logging
                onCheckedChanged: appSettings.logging = checked
            }
        }

        VerticalScrollDecorator {}
    }
}
