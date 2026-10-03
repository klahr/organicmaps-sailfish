#pragma once

#include <QMediaPlayer>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

#include <map>
#include <string>
#include <vector>

class QDBusPendingCallWatcher;

namespace sailfish
{
// Speaks turn notifications, the role of the Android TtsPlayer. Sailfish OS has no speech engine of its own, so
// this uses Speech Note (natural Piper voices) over D-Bus when it has a voice for the language, or else a speech
// synthesizer program installed on the device, as Pure Maps does.
class VoiceGuide : public QObject
{
  Q_OBJECT

public:
  struct Engine;

  explicit VoiceGuide(QObject * parent = nullptr);
  ~VoiceGuide() override;

  bool IsAvailable() const { return !m_language.empty(); }
  // The turn notifications language being spoken, empty when there is no voice.
  std::string const & Language() const { return m_language; }
  // Core turn notifications languages with a voice, in the core order.
  std::vector<std::string> Languages() const;
  bool IsSpeechNoteInstalled() const { return m_speechNoteInstalled; }
  bool HasSpeechNoteVoice(std::string const & language) const;

  // The language to speak when it has a voice; otherwise the app language, any Speech Note voice or English.
  void SetPreferredLanguage(std::string const & preferred, std::string const & appLanguage);
  // Looks up the voices again, e.g. after Speech Note or a voice was installed. Changed() follows.
  void Refresh();
  // Opens Speech Note, to download voices.
  void OpenSpeechNote();

  // Replaces what is being said, as the Android TtsPlayer does.
  void Speak(QStringList const & texts);
  void Stop();

signals:
  void Changed();

private slots:
  void OnSpeechNoteFinished(int task);

private:
  // An installed speech synthesizer program.
  struct Program
  {
    Engine const * m_engine;
    QString m_path;
  };

  void ChooseLanguage();
  void OnSpeechNoteLanguages(QDBusPendingCallWatcher * watcher);
  void SpeakNextWithSpeechNote();
  void SynthesizeNext();
  void OnSynthesized(int exitCode, QProcess::ExitStatus status);
  void OnPlayerStateChanged(QMediaPlayer::State state);

  std::string m_preferredLanguage;
  std::string m_appLanguage;
  std::string m_language;
  QStringList m_queue;

  bool m_speechNoteInstalled = false;
  // Core language -> Speech Note language with a voice.
  std::map<std::string, QString> m_speechNoteVoices;
  // Core language -> the first program that speaks it.
  std::map<std::string, Program> m_programVoices;

  // Set when the language is spoken by Speech Note, with the task being spoken.
  QString m_speechNoteLanguage;
  int m_speechNoteTask = -1;

  // Otherwise a program writes a WAV file, which is played here: not all of them can play by themselves.
  Engine const * m_engine = nullptr;
  QString m_program;
  QString m_voice;
  QString m_wavFile;
  QProcess m_process;
  // The Sailfish media player also acquires the audio resource, without which the policy keeps it silent.
  QMediaPlayer m_player;
};
}  // namespace sailfish
