#include "sailfish/app_info.hpp"

#include "sailfish/framework_access.hpp"

#include "map/framework.hpp"

#include "indexer/categories_holder.hpp"

#include "platform/duration.hpp"
#include "platform/localization.hpp"
#include "platform/platform.hpp"

#include "base/timer.hpp"

#include <QDateTime>
#include <QGuiApplication>
#include <QInputMethod>
#include <QLocale>

#include <algorithm>

namespace sailfish
{
QString Localized(QString const & key, QStringList const & args)
{
  QString s = QString::fromStdString(platform::GetLocalizedString(key.toStdString()));
  for (int i = 0; i < args.size(); ++i)
  {
    s.replace(QStringLiteral("%%1$@").arg(i + 1), args[i]);
    s.replace(QStringLiteral("%%1$d").arg(i + 1), args[i]);
  }
  if (!args.isEmpty())
    s.replace(QStringLiteral("%@"), args[0]).replace(QStringLiteral("%d"), args[0]);
  return s;
}

QString ToQString(std::string_view s)
{
  return QString::fromUtf8(s.data(), static_cast<int>(s.size()));
}

QString FormatDuration(long seconds)
{
  return QString::fromStdString(platform::Duration(static_cast<unsigned long>(seconds)).GetHoursMinutesString());
}

QString FormatSize(qint64 bytes)
{
  qint64 constexpr kMb = 1024 * 1024;
  qint64 constexpr kGb = 1024 * kMb;
  if (bytes < kGb)
    return QString::number(std::max<qint64>(1, (bytes + kMb / 2) / kMb)) + ' ' + Localized("mb");
  return QLocale::system().toString(static_cast<double>(bytes) / kGb, 'f', 1) + ' ' + Localized("gb");
}

std::string GetInputLocale()
{
  std::string locale = QGuiApplication::inputMethod()->locale().name().replace('_', '-').toStdString();
  if (CategoriesHolder::MapLocaleToInteger(locale) == CategoriesHolder::kUnsupportedLocaleCode)
  {
    // Try the language without the region, e.g. sv for sv-SE.
    locale = locale.substr(0, locale.find('-'));
    if (CategoriesHolder::MapLocaleToInteger(locale) == CategoriesHolder::kUnsupportedLocaleCode)
      locale = "en";
  }
  return locale;
}

QString AppInfo::version() const
{
  return QString::fromStdString(GetPlatform().Version());
}

QDate AppInfo::dataVersion() const
{
  auto const yymmdd = static_cast<uint32_t>(GetFramework().GetCurrentDataVersion());
  return QDateTime::fromTime_t(static_cast<uint>(base::YYMMDDToSecondsSinceEpoch(yymmdd)), Qt::UTC).date();
}
}  // namespace sailfish
