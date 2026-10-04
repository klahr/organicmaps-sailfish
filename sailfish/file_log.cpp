#include "sailfish/file_log.hpp"

#include "base/logging.hpp"
#include "base/src_point.hpp"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

#include <fstream>
#include <mutex>

namespace sailfish::file_log
{
namespace
{
// Older lines are dropped when the app starts with a bigger file.
qint64 constexpr kMaxSize = 5 * 1024 * 1024;

std::mutex g_mutex;
std::ofstream g_file;
QtMessageHandler g_qtHandler = nullptr;

void Write(char const * level, std::string const & where, std::string const & message)
{
  auto const time = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-ddTHH:mm:ss.zzz")).toStdString();
  std::lock_guard lock(g_mutex);
  if (g_file.is_open())
    g_file << time << ' ' << level << ' ' << where << ' ' << message << std::endl;
}

void LogCore(base::LogLevel level, base::SrcPoint const & srcPoint, std::string const & message)
{
  Write(base::ToString(level).c_str(), DebugPrint(srcPoint), message);
  base::LogMessageDefault(level, srcPoint, message);
}

void LogQt(QtMsgType type, QMessageLogContext const & context, QString const & message)
{
  char const * const levels[] = {"QDEBUG", "QWARNING", "QCRITICAL", "QFATAL", "QINFO"};
  auto const where = context.file ? std::string(context.file) + ':' + std::to_string(context.line) : std::string();
  Write(type >= 0 && type < static_cast<int>(std::size(levels)) ? levels[type] : "QT", where, message.toStdString());
  g_qtHandler(type, context, message);
}
}  // namespace

QString Path()
{
  return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/logs/organicmaps.log");
}

void Enable(bool enabled)
{
  if (enabled == IsEnabled())
    return;
  if (enabled)
  {
    auto const path = Path();
    QDir().mkpath(QFileInfo(path).path());
    auto mode = std::ios::out | std::ios::app;
    if (QFileInfo(path).size() > kMaxSize)
      mode = std::ios::out | std::ios::trunc;
    {
      std::lock_guard lock(g_mutex);
      g_file.open(path.toStdString(), mode);
    }
    base::SetLogMessageFn(&LogCore);
    base::g_LogLevel = base::LDEBUG;
    g_qtHandler = qInstallMessageHandler(&LogQt);
    LOG(LINFO, ("Logging to", path.toStdString()));
  }
  else
  {
    qInstallMessageHandler(g_qtHandler);
    base::SetLogMessageFn(&base::LogMessageDefault);
    base::g_LogLevel = base::GetDefaultLogLevel();
    std::lock_guard lock(g_mutex);
    g_file.close();
  }
}

bool IsEnabled()
{
  std::lock_guard lock(g_mutex);
  return g_file.is_open();
}
}  // namespace sailfish::file_log
