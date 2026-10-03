import QtQuick 2.6
import Sailfish.Silica 1.0

// Voice instructions, like the Android VoiceInstructionsSettingsFragment. Sailfish OS has no speech engine, so
// this also leads to Speech Note, which speaks with natural voices.
Page {
    id: page

    // MapItem.routing.
    property QtObject routing
    property bool testPlayed

    // The wanted language has a Speech Note voice.
    readonly property bool wantedHasSpeechNoteVoice: {
        var languages = routing.voiceLanguages
        for (var i = 0; i < languages.length; ++i)
            if (languages[i].name === routing.wantedVoiceLanguageName)
                return languages[i].speechNote
        return false
    }

    allowedOrientations: Orientation.All

    // Back from Speech Note or the store, there may be new voices.
    onStatusChanged: if (status === PageStatus.Activating) routing.refreshVoice()
    Connections {
        target: Qt.application
        onActiveChanged: if (Qt.application.active && page.status === PageStatus.Active) routing.refreshVoice()
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: appInfo.localized("pref_tts_enable_title")
            }

            TextSwitch {
                text: appInfo.localized("pref_tts_enable_title")
                description: routing.voiceAvailable ? "" : appInfo.localized("pref_tts_unavailable")
                enabled: routing.voiceAvailable
                automaticCheck: false
                checked: routing.voiceEnabled
                onClicked: routing.voiceEnabled = !routing.voiceEnabled
            }
            ComboBox {
                id: languageBox
                visible: routing.voiceLanguages.length > 0
                enabled: routing.voiceEnabled
                label: appInfo.localized("pref_tts_language_title")
                currentIndex: {
                    var languages = routing.voiceLanguages
                    for (var i = 0; i < languages.length; ++i)
                        if (languages[i].code === routing.voiceLanguage)
                            return i
                    return -1
                }
                menu: ContextMenu {
                    Repeater {
                        model: routing.voiceLanguages
                        // Speech Note voices sound natural, the others are plain synthesizers.
                        MenuItem {
                            text: modelData.speechNote ? modelData.name + " · Speech Note" : modelData.name
                            onClicked: routing.voiceLanguage = modelData.code
                        }
                    }
                }
            }
            TextSwitch {
                enabled: routing.voiceEnabled
                text: appInfo.localized("pref_tts_street_names_title")
                description: appInfo.localized("pref_tts_street_names_description")
                automaticCheck: false
                checked: routing.announceStreets
                onClicked: routing.announceStreets = !routing.announceStreets
            }
            ValueButton {
                enabled: routing.voiceEnabled
                label: appInfo.localized("pref_tts_test_voice_title")
                description: page.testPlayed ? appInfo.localized("pref_tts_playing_test_voice") : ""
                onClicked: {
                    page.testPlayed = true
                    routing.testVoice()
                }
            }

            SectionHeader {
                text: "Speech Note"
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                text: {
                    if (!routing.speechNoteInstalled)
                        return qsTr("Speech Note speaks the instructions with natural voices. Install it, then download a voice for %1 in it.")
                               .arg(routing.wantedVoiceLanguageName)
                               + (routing.voiceAvailable ? "" : " " + qsTr("eSpeak NG, mimic or flite also work, with plainer voices."))
                    if (!page.wantedHasSpeechNoteVoice)
                        return qsTr("Download a voice for %1 in Speech Note to hear the instructions in it.")
                               .arg(routing.wantedVoiceLanguageName)
                    return qsTr("The instructions are spoken by Speech Note. It has more voices to download.")
                }
            }
            Item {
                width: 1
                height: Theme.paddingLarge
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: routing.speechNoteInstalled ? qsTr("Open Speech Note") : qsTr("Get Speech Note")
                onClicked: {
                    if (routing.speechNoteInstalled)
                        routing.openSpeechNote()
                    else
                        Qt.openUrlExternally("https://openrepos.net/content/mkiol/speech-note")
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
