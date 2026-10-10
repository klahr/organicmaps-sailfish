#include "sailfish/stop_departures.hpp"

#include "sailfish/helpers.hpp"

#include "departures/trafiklab.hpp"

#include "platform/platform.hpp"

#include <QPointer>
#include <QVariantMap>

#include <chrono>

namespace sailfish
{
namespace
{
// Shown at the most, which also covers the next hour at busy stops.
size_t constexpr kMaxDepartures = 12;

QString ModeIcon(departures::TransportMode mode)
{
  using departures::TransportMode;
  switch (mode)
  {
  case TransportMode::Bus: return QStringLiteral("ic_20px_route_planning_bus.svg");
  case TransportMode::Tram: return QStringLiteral("ic_20px_route_planning_tram.svg");
  case TransportMode::Metro: return QStringLiteral("ic_20px_route_planning_metro.webp");
  case TransportMode::Train: return QStringLiteral("ic_20px_route_planning_train.webp");
  case TransportMode::Ferry:
  case TransportMode::Other: return {};
  }
  return {};
}

QVariantList ToVariant(departures::Departures const & result)
{
  using namespace std::chrono;
  QVariantList list;
  for (auto const & d : result.m_departures)
  {
    if (static_cast<size_t>(list.size()) == kMaxDepartures)
      break;
    auto const time = d.GetTime();
    // Gone already, by the provider's clock.
    if (time + 1min < result.m_now)
      continue;
    hh_mm_ss const clock{time - floor<days>(time)};
    list.append(QVariantMap{
        {"line", QString::fromStdString(d.m_line)},
        {"destination", QString::fromStdString(d.m_destination)},
        {"icon", ModeIcon(d.m_mode)},
        {"minutes", static_cast<int>(std::max(floor<minutes>(time - result.m_now).count(), minutes::rep{0}))},
        {"time", QStringLiteral("%1:%2")
                     .arg(clock.hours().count(), 2, 10, QLatin1Char('0'))
                     .arg(clock.minutes().count(), 2, 10, QLatin1Char('0'))},
        {"delay", static_cast<int>(round<minutes>(time - d.m_scheduled).count())},
        {"platform", QString::fromStdString(d.m_platform)},
        {"canceled", d.m_canceled},
    });
  }
  return list;
}
}  // namespace

StopDepartures::StopDepartures(QObject * parent) : QObject(parent) {}

StopDepartures::~StopDepartures() = default;

void StopDepartures::SetStop(double lat, double lon, std::string name)
{
  // Read each time, so that a key entered in the settings applies to the next stop.
  auto key = LoadSetting(kTrafiklabKeySetting, std::string());
  if (key != m_providerKey)
  {
    m_providerKey = key;
    m_provider = key.empty() ? nullptr : std::make_shared<departures::TrafiklabProvider>(std::move(key));
  }
  if (!m_provider || !m_provider->Covers({lat, lon}))
  {
    Clear();
    return;
  }
  // The place page also updates for the same place, e.g. when it is bookmarked.
  if (m_active && lat == m_lat && lon == m_lon && name == m_name)
    return;
  m_active = true;
  m_lat = lat;
  m_lon = lon;
  m_name = std::move(name);
  m_departures.clear();
  m_status = Ok;
  auto const attribution = m_provider->GetAttribution();
  m_attribution = QString::fromUtf8(attribution.data(), static_cast<int>(attribution.size()));
  refresh();
}

void StopDepartures::Clear()
{
  ++m_requestId;
  if (!m_active)
    return;
  m_active = false;
  m_loading = false;
  m_departures.clear();
  emit changed();
}

void StopDepartures::refresh()
{
  if (!m_active)
    return;
  m_loading = true;
  emit changed();

  QPointer<StopDepartures> self(this);
  auto const id = ++m_requestId;
  GetPlatform().RunTask(Platform::Thread::Network,
                        [self, id, provider = m_provider, stop = departures::Stop{{m_lat, m_lon}, m_name}]
  {
    auto const result = provider->Request(stop);
    GetPlatform().RunTask(Platform::Thread::Gui, [self, id, result]
    {
      if (!self || id != self->m_requestId)
        return;
      self->m_loading = false;
      self->m_status = static_cast<int>(result.m_status);
      // Earlier departures stay shown when a refresh fails.
      if (result.m_status == departures::Status::Ok)
        self->m_departures = ToVariant(result);
      emit self->changed();
    });
  });
}
}  // namespace sailfish
