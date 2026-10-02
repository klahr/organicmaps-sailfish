#include "sailfish/app_info.hpp"

#include "sailfish/framework_access.hpp"

#include "map/framework.hpp"

#include "platform/localization.hpp"
#include "platform/platform.hpp"

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

QString AppInfo::version() const
{
  return QString::fromStdString(GetPlatform().Version());
}

QDate AppInfo::dataVersion() const
{
  // The version is yymmdd.
  auto const v = GetFramework().GetStorage().GetCurrentDataVersion();
  return QDate(2000 + static_cast<int>(v / 10000), static_cast<int>(v / 100 % 100), static_cast<int>(v % 100));
}
}  // namespace sailfish
