#pragma once

#include <QDate>
#include <QObject>
#include <QString>
#include <QStringList>

namespace sailfish
{
// A shared UI string from data/strings, with its iOS style placeholders (%@, %1$@) filled in.
QString Localized(QString const & key, QStringList const & args = {});

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
};
}  // namespace sailfish
