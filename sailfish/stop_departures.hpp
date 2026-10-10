#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

#include <memory>
#include <string>
#include <string_view>

namespace departures
{
class Provider;
}

namespace sailfish
{
// Holds the departures provider's key, so that live departures are opt-in.
std::string_view constexpr kTrafiklabKeySetting = "SailfishTrafiklabKey";

// Live departures at the place page's stop, from an online provider.
class StopDepartures : public QObject
{
  Q_OBJECT
  // A provider covers the stop.
  Q_PROPERTY(bool available READ available NOTIFY changed)
  Q_PROPERTY(bool loading READ loading NOTIFY changed)
  Q_PROPERTY(int status READ status NOTIFY changed)
  // {line, destination, icon, minutes, time, delay, platform, canceled}; minutes until it leaves, as the
  // provider's clock tells.
  Q_PROPERTY(QVariantList departures READ departures NOTIFY changed)
  Q_PROPERTY(QString attribution READ attribution NOTIFY changed)

public:
  // Same as departures::Status.
  enum Status
  {
    Ok,
    StopNotFound,
    NetworkError,
    Unauthorized,
    ServerError,
  };
  Q_ENUM(Status)

  explicit StopDepartures(QObject * parent = nullptr);
  ~StopDepartures() override;

  bool available() const { return m_active; }
  bool loading() const { return m_loading; }
  int status() const { return m_status; }
  QVariantList departures() const { return m_departures; }
  QString attribution() const { return m_attribution; }

  Q_INVOKABLE void refresh();

  // Clears without a provider for the position.
  void SetStop(double lat, double lon, std::string name);
  void Clear();

signals:
  void changed();

private:
  std::shared_ptr<departures::Provider> m_provider;
  std::string m_providerKey;
  bool m_active = false;
  double m_lat = 0.0;
  double m_lon = 0.0;
  std::string m_name;
  // Results of earlier requests are dropped.
  quint64 m_requestId = 0;
  bool m_loading = false;
  int m_status = Ok;
  QVariantList m_departures;
  QString m_attribution;
};
}  // namespace sailfish
