#include "sailfish/voice_guide.hpp"

#include "platform/languages.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>
#include <QDir>
#include <QStandardPaths>
#include <QUrl>

namespace sailfish
{
struct VoiceGuide::Engine
{
  char const * m_program;
  // The voice of the program for a turn notifications language, empty when it doesn't speak it.
  QString (*m_voice)(std::string const & language);
  QStringList (*m_arguments)(QString const & voice, QString const & text, QString const & wavFile);
};

namespace
{
QString const kSpeechNoteService = QStringLiteral("org.mkiol.Speech");

QDBusMessage SpeechNoteCall(QString const & method)
{
  return QDBusMessage::createMethodCall(kSpeechNoteService, QStringLiteral("/"), kSpeechNoteService, method);
}

// Speech Note names languages like "en" or "pt_BR"; the core like "en" or "pt-BR".
QString SpeechNoteCode(std::string const & language)
{
  return QString::fromStdString(language).replace('-', '_');
}

QString EspeakVoice(std::string const & language)
{
  // Codes of the core languages that eSpeak names differently.
  if (language == "zh-Hans" || language == "zh-Hant")
    return QStringLiteral("cmn");
  if (language.starts_with("yue"))
    return QStringLiteral("yue");
  if (language == "es-MX")
    return QStringLiteral("es-419");
  return QString::fromStdString(language).toLower();
}

QString EnglishOnly(std::string const & language)
{
  return language == "en" ? QStringLiteral("en") : QString();
}

// The languages of the SVOX Pico voices.
QString PicoVoice(std::string const & language)
{
  for (char const * voice : {"en-US", "de-DE", "es-ES", "fr-FR", "it-IT"})
    if (language == std::string_view(voice, 2))
      return voice;
  return {};
}

QStringList EspeakArguments(QString const & voice, QString const & text, QString const & wavFile)
{
  return {"-v", voice, "-w", wavFile, text};
}

QStringList MimicArguments(QString const &, QString const & text, QString const & wavFile)
{
  return {"-t", text, "-o", wavFile};
}

// In order of preference, as in Pure Maps.
VoiceGuide::Engine const kEngines[] = {
    {"espeak-ng", EspeakVoice, EspeakArguments},
    {"espeak", EspeakVoice, EspeakArguments},
    {"mimic", EnglishOnly, MimicArguments},
    {"flite", EnglishOnly, MimicArguments},
    {"pico2wave", PicoVoice, [](QString const & voice, QString const & text, QString const & wavFile)
{ return QStringList{"-l", voice, "-w", wavFile, text}; }},
};
}  // namespace

VoiceGuide::VoiceGuide(QObject * parent) : QObject(parent)
{
  connect(&m_process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this,
          &VoiceGuide::OnSynthesized);
  connect(&m_player, &QMediaPlayer::stateChanged, this, &VoiceGuide::OnPlayerStateChanged);
  connect(&m_player, static_cast<void (QMediaPlayer::*)(QMediaPlayer::Error)>(&QMediaPlayer::error), this,
          [this] { LOG(LWARNING, ("Speech playback failed:", m_player.errorString().toStdString())); });
  QDBusConnection::sessionBus().connect(kSpeechNoteService, QStringLiteral("/"), kSpeechNoteService,
                                        QStringLiteral("TtsPlaySpeechFinished"), this, SLOT(OnSpeechNoteFinished(int)));
}

VoiceGuide::~VoiceGuide()
{
  Stop();
}

std::vector<std::string> VoiceGuide::Languages() const
{
  std::vector<std::string> languages;
  for (auto const & lang : routing::turns::sound::kLanguageList)
  {
    std::string const code(lang.first);
    if (m_speechNoteVoices.contains(code) || m_programVoices.contains(code))
      languages.push_back(code);
  }
  return languages;
}

bool VoiceGuide::HasSpeechNoteVoice(std::string const & language) const
{
  return m_speechNoteVoices.contains(language);
}

void VoiceGuide::SetPreferredLanguage(std::string const & preferred, std::string const & appLanguage)
{
  m_preferredLanguage = preferred;
  m_appLanguage = appLanguage;
  ChooseLanguage();
}

void VoiceGuide::Refresh()
{
  m_programVoices.clear();
  for (auto const & lang : routing::turns::sound::kLanguageList)
  {
    std::string const code(lang.first);
    for (auto const & engine : kEngines)
    {
      if (!engine.m_voice(code).isEmpty() && !QStandardPaths::findExecutable(engine.m_program).isEmpty())
      {
        m_programVoices.emplace(code, &engine);
        break;
      }
    }
  }
  ChooseLanguage();

  // Asks an installed Speech Note for its voices; this starts its service. Not installed, the call fails.
  auto const watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(
      QDBusMessage::createMethodCall(kSpeechNoteService, QStringLiteral("/"),
                                     QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("Get"))
      << kSpeechNoteService << QStringLiteral("TtsLangs")));
  connect(watcher, &QDBusPendingCallWatcher::finished, this, &VoiceGuide::OnSpeechNoteLanguages);
}

void VoiceGuide::OnSpeechNoteLanguages(QDBusPendingCallWatcher * watcher)
{
  watcher->deleteLater();
  QDBusPendingReply<QDBusVariant> const reply = *watcher;
  m_speechNoteVoices.clear();
  m_speechNoteInstalled = !reply.isError();
  if (reply.isError())
  {
    LOG(LINFO, ("No Speech Note:", reply.error().message().toStdString()));
  }
  else
  {
    QVariantMap languages;
    reply.value().variant().value<QDBusArgument>() >> languages;
    for (auto const & lang : routing::turns::sound::kLanguageList)
    {
      std::string const code(lang.first);
      auto const speechNoteCode = SpeechNoteCode(code);
      if (languages.contains(speechNoteCode))
        m_speechNoteVoices.emplace(code, speechNoteCode);
    }
  }
  ChooseLanguage();
}

void VoiceGuide::ChooseLanguage()
{
  Stop();
  auto const languages = Languages();
  auto const has = [&languages](std::string const & code) { return base::IsExist(languages, code); };
  std::string language;
  if (has(m_preferredLanguage))
    language = m_preferredLanguage;
  else if (has(m_appLanguage))
    language = m_appLanguage;
  else if (!m_speechNoteVoices.empty())
    language = m_speechNoteVoices.begin()->first;
  else if (has("en"))
    language = "en";

  m_language = language;
  m_speechNoteLanguage.clear();
  m_engine = nullptr;
  if (auto const it = m_speechNoteVoices.find(language); it != m_speechNoteVoices.end())
  {
    m_speechNoteLanguage = it->second;
    LOG(LINFO, ("Voice instructions in", language, "with Speech Note"));
  }
  else if (auto const it = m_programVoices.find(language); it != m_programVoices.end())
  {
    m_engine = it->second;
    m_program = QStandardPaths::findExecutable(m_engine->m_program);
    m_voice = m_engine->m_voice(language);
    LOG(LINFO, ("Voice instructions in", language, "with", m_program.toStdString()));
  }
  else
  {
    LOG(LINFO, ("No voice for voice instructions"));
  }
  emit Changed();
}

void VoiceGuide::OpenSpeechNote()
{
  QDBusConnection::sessionBus().asyncCall(
      QDBusMessage::createMethodCall(QStringLiteral("org.mkiol.dsnote"), QStringLiteral("/org/mkiol/dsnote"),
                                     QStringLiteral("org.freedesktop.Application"), QStringLiteral("Activate"))
      << QVariantMap{});
}

void VoiceGuide::Speak(QStringList const & texts)
{
  if (!IsAvailable() || texts.isEmpty())
    return;
  Stop();
  m_queue = texts;
  if (!m_speechNoteLanguage.isEmpty())
    SpeakNextWithSpeechNote();
  else
    SynthesizeNext();
}

void VoiceGuide::Stop()
{
  m_queue.clear();
  if (m_speechNoteTask >= 0)
  {
    QDBusConnection::sessionBus().asyncCall(SpeechNoteCall(QStringLiteral("TtsStopSpeech")) << m_speechNoteTask);
    m_speechNoteTask = -1;
  }
  if (m_process.state() != QProcess::NotRunning)
  {
    m_process.kill();
    m_process.waitForFinished(1000);
  }
  m_player.stop();
}

void VoiceGuide::SpeakNextWithSpeechNote()
{
  if (m_queue.isEmpty() || m_speechNoteTask >= 0)
    return;
  auto const watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(
      SpeechNoteCall(QStringLiteral("TtsPlaySpeech")) << m_queue.takeFirst() << m_speechNoteLanguage));
  connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher * call)
  {
    call->deleteLater();
    QDBusPendingReply<int> const reply = *call;
    if (reply.isError())
    {
      LOG(LWARNING, ("Speech Note failed:", reply.error().message().toStdString()));
      return;
    }
    // -1 when Speech Note is busy with something else.
    m_speechNoteTask = reply.value();
    if (m_speechNoteTask < 0)
      LOG(LWARNING, ("Speech Note didn't take the voice instruction"));
  });
}

void VoiceGuide::OnSpeechNoteFinished(int task)
{
  if (task != m_speechNoteTask)
    return;
  m_speechNoteTask = -1;
  SpeakNextWithSpeechNote();
}

void VoiceGuide::SynthesizeNext()
{
  if (m_queue.isEmpty() || m_process.state() != QProcess::NotRunning || m_player.state() == QMediaPlayer::PlayingState)
    return;
  // Two files take turns, so that a new one never replaces the media being played.
  QString const dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
  QDir().mkpath(dir);
  m_wavFile = dir + (m_wavFile.endsWith("speech0.wav") ? "/speech1.wav" : "/speech0.wav");
  m_process.start(m_program, m_engine->m_arguments(m_voice, m_queue.takeFirst(), m_wavFile));
}

void VoiceGuide::OnSynthesized(int exitCode, QProcess::ExitStatus status)
{
  if (status != QProcess::NormalExit || exitCode != 0)
  {
    LOG(LWARNING, ("Speech synthesizer failed:", m_program.toStdString(), exitCode,
                   m_process.readAllStandardError().toStdString()));
    SynthesizeNext();
    return;
  }
  m_player.setMedia(QUrl::fromLocalFile(m_wavFile));
  m_player.play();
}

void VoiceGuide::OnPlayerStateChanged(QMediaPlayer::State state)
{
  LOG(LINFO, ("Speech player state", static_cast<int>(state)));
  if (state == QMediaPlayer::StoppedState)
    SynthesizeNext();
}
}  // namespace sailfish
