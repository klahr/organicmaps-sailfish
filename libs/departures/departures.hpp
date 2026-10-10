#pragma once

#include "geometry/latlon.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Live public transport departures from online providers.
namespace departures
{
// Wall-clock time at the stop: providers report local times, which are compared with the provider's own clock.
using LocalTime = std::chrono::local_seconds;

enum class TransportMode : uint8_t
{
  Bus,
  Tram,
  Metro,
  Train,
  Ferry,
  Other,
};

struct Departure
{
  LocalTime GetTime() const { return m_realtime.value_or(m_scheduled); }

  std::string m_line;
  std::string m_destination;
  TransportMode m_mode = TransportMode::Other;
  LocalTime m_scheduled;
  // Without a prediction only the timetable is known.
  std::optional<LocalTime> m_realtime;
  std::string m_platform;
  bool m_canceled = false;
};

enum class Status : uint8_t
{
  Ok,
  StopNotFound,
  NetworkError,
  Unauthorized,
  ServerError,
};

struct Departures
{
  Status m_status = Status::Ok;
  // The provider's clock when it answered.
  LocalTime m_now;
  std::vector<Departure> m_departures;
};

struct Stop
{
  ms::LatLon m_latLon;
  std::string m_name;
};

class Provider
{
public:
  virtual ~Provider() = default;

  virtual bool Covers(ms::LatLon const & latLon) const = 0;
  // The data license requires showing it with the departures.
  virtual std::string_view GetAttribution() const = 0;
  // Blocks on network requests, so it is called off the GUI thread. Thread-safe.
  virtual Departures Request(Stop const & stop) = 0;
};

// "YYYY-MM-DDTHH:MM[:SS]", ignoring any fraction or time zone after it.
std::optional<LocalTime> ParseLocalTime(std::string_view s);

std::string DebugPrint(TransportMode mode);
std::string DebugPrint(Status status);
}  // namespace departures
