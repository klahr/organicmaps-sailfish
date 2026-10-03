#pragma once

#include <QDate>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTime>

#include <string>
#include <string_view>

namespace sailfish
{
// A shared UI string from data/strings, with its placeholders (%@, %1$@, %d, %1$d) filled in.
QString Localized(QString const & key, QStringList const & args = {});
QString ToQString(std::string_view s);
// "1 h 5 min", rounded to the nearest minute.
QString FormatDuration(long seconds);
// The Sailfish clock setting, which is set apart from the language, e.g. English with a 24-hour clock.
bool Is24HourClock();
// A clock time in that format.
QString FormatTime(QTime const & time);
// "45 MB" or "1.2 GB", like StringUtils.getFileSizeString() on Android.
QString FormatSize(qint64 bytes);
// The search language follows the keyboard, like on the other platforms; English without categories in it.
std::string GetInputLocale();

// App details and shared strings for QML, available as the appInfo context property.
class AppInfo : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString version READ version CONSTANT)
  // Date of the downloaded map data, like Framework.getDataVersion() on Android.
  Q_PROPERTY(QDate dataVersion READ dataVersion CONSTANT)

public:
  using QObject::QObject;

  QString version() const;
  QDate dataVersion() const;

  Q_INVOKABLE QString localized(QString const & key, QStringList const & args = {}) const
  {
    return Localized(key, args);
  }
  Q_INVOKABLE QString formatSize(qint64 bytes) const { return FormatSize(bytes); }
};
}  // namespace sailfish
