#pragma once

#include "departures/departures.hpp"

#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace departures
{
// Trafiklab Realtime APIs: all public transport in Sweden, with a key from trafiklab.se.
class TrafiklabProvider : public Provider
{
public:
  explicit TrafiklabProvider(std::string key) : m_key(std::move(key)) {}

  bool Covers(ms::LatLon const & latLon) const override;
  std::string_view GetAttribution() const override { return "Trafiklab.se"; }
  Departures Request(Stop const & stop) override;

private:
  // The stop group id, found by the stop's name, since the API has no lookup by position.
  std::optional<std::string> FindStopGroup(Stop const & stop, Status & status);

  std::string const m_key;
  std::mutex m_mutex;
  // By the stop's name and position, so that refreshes look it up once.
  std::unordered_map<std::string, std::string> m_stopGroups;
};

namespace trafiklab
{
// Of the groups matching a name, the one with a stop nearest to |latLon|, if near enough.
std::optional<std::string> ParseNearestStopGroup(std::string_view json, ms::LatLon const & latLon);
bool ParseDepartures(std::string_view json, Departures & departures);
}  // namespace trafiklab
}  // namespace departures
