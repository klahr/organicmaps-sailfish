#include "departures/trafiklab.hpp"

#include "platform/http_client.hpp"

#include "coding/url.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <limits>
#include <vector>

#include <glaze/json.hpp>

namespace departures
{
namespace trafiklab_json
{
struct ChildStop
{
  double lat = 0.0;
  double lon = 0.0;
};

struct StopGroup
{
  std::string id;
  std::vector<ChildStop> stops;
};

struct StopsResponse
{
  std::vector<StopGroup> stop_groups;
};

struct Place
{
  std::string name;
};

struct Route
{
  std::string designation;
  std::string transport_mode;
  std::string direction;
  std::optional<Place> destination;
};

struct Platform
{
  std::string designation;
};

struct Departure
{
  std::string scheduled;
  std::optional<std::string> realtime;
  bool canceled = false;
  bool is_realtime = false;
  Route route;
  std::optional<Platform> scheduled_platform;
  std::optional<Platform> realtime_platform;
};

struct DeparturesResponse
{
  std::string timestamp;
  std::vector<Departure> departures;
};
}  // namespace trafiklab_json

namespace
{
std::string_view constexpr kBaseUrl = "https://realtime-api.trafiklab.se/v1/";
double constexpr kTimeoutSec = 10.0;
// Between the stop on the map and the nearest stop of a group with its name.
double constexpr kMaxStopDistanceM = 500.0;

glz::opts constexpr kJsonOpts{.error_on_unknown_keys = false};

TransportMode ToMode(std::string_view mode)
{
  if (mode == "BUS")
    return TransportMode::Bus;
  if (mode == "TRAM")
    return TransportMode::Tram;
  if (mode == "METRO")
    return TransportMode::Metro;
  if (mode == "TRAIN")
    return TransportMode::Train;
  if (mode == "FERRY" || mode == "BOAT")
    return TransportMode::Ferry;
  return TransportMode::Other;
}

Status ToStatus(int httpCode)
{
  if (httpCode == 401 || httpCode == 403)
    return Status::Unauthorized;
  if (httpCode == 404)
    return Status::StopNotFound;
  // Codes below 100 are transport errors.
  return httpCode < 100 ? Status::NetworkError : Status::ServerError;
}

std::optional<std::string> Get(std::string const & url, Status & status)
{
  platform::HttpClient request(url);
  request.SetTimeout(kTimeoutSec);
  if (!request.RunHttpRequest() || request.ErrorCode() != 200)
  {
    status = ToStatus(request.ErrorCode());
    LOG(LWARNING, ("Trafiklab request failed:", request.ErrorCode()));
    return {};
  }
  return request.ServerResponse();
}
}  // namespace

bool TrafiklabProvider::Covers(ms::LatLon const & latLon) const
{
  // Sweden's bounding box.
  return latLon.m_lat >= 55.0 && latLon.m_lat <= 69.1 && latLon.m_lon >= 10.9 && latLon.m_lon <= 24.2;
}

std::optional<std::string> TrafiklabProvider::FindStopGroup(Stop const & stop, Status & status)
{
  auto const cacheKey = stop.m_name + '|' + DebugPrint(stop.m_latLon);
  {
    std::lock_guard lock(m_mutex);
    if (auto const it = m_stopGroups.find(cacheKey); it != m_stopGroups.end())
      return it->second;
  }

  auto const json = Get(std::string(kBaseUrl) + "stops/name/" + url::UrlEncode(stop.m_name) + "?key=" + m_key, status);
  if (!json)
    return {};
  auto id = trafiklab::ParseNearestStopGroup(*json, stop.m_latLon);
  if (!id)
  {
    status = Status::StopNotFound;
    return {};
  }

  std::lock_guard lock(m_mutex);
  m_stopGroups.emplace(cacheKey, *id);
  return id;
}

Departures TrafiklabProvider::Request(Stop const & stop)
{
  Departures result;
  if (stop.m_name.empty())
  {
    result.m_status = Status::StopNotFound;
    return result;
  }

  auto const id = FindStopGroup(stop, result.m_status);
  if (!id)
    return result;
  auto const json = Get(std::string(kBaseUrl) + "departures/" + url::UrlEncode(*id) + "?key=" + m_key, result.m_status);
  if (json && !trafiklab::ParseDepartures(*json, result))
    result.m_status = Status::ServerError;
  return result;
}

namespace trafiklab
{
std::optional<std::string> ParseNearestStopGroup(std::string_view json, ms::LatLon const & latLon)
{
  trafiklab_json::StopsResponse response;
  if (auto const error = glz::read<kJsonOpts>(response, json); error)
  {
    LOG(LWARNING, ("Can't parse Trafiklab stops:", glz::format_error(error, json)));
    return {};
  }

  std::optional<std::string> nearest;
  double nearestDistance = kMaxStopDistanceM;
  for (auto const & group : response.stop_groups)
  {
    for (auto const & stop : group.stops)
    {
      double const distance = ms::DistanceOnEarth(latLon, {stop.lat, stop.lon});
      if (distance <= nearestDistance)
      {
        nearestDistance = distance;
        nearest = group.id;
      }
    }
  }
  return nearest;
}

bool ParseDepartures(std::string_view json, Departures & departures)
{
  trafiklab_json::DeparturesResponse response;
  if (auto const error = glz::read<kJsonOpts>(response, json); error)
  {
    LOG(LWARNING, ("Can't parse Trafiklab departures:", glz::format_error(error, json)));
    return false;
  }
  auto const now = ParseLocalTime(response.timestamp);
  if (!now)
    return false;

  departures.m_now = *now;
  departures.m_departures.clear();
  for (auto const & d : response.departures)
  {
    auto const scheduled = ParseLocalTime(d.scheduled);
    if (!scheduled)
      continue;
    Departure & departure = departures.m_departures.emplace_back();
    departure.m_line = d.route.designation;
    departure.m_destination = d.route.destination ? d.route.destination->name : d.route.direction;
    departure.m_mode = ToMode(d.route.transport_mode);
    departure.m_scheduled = *scheduled;
    if (d.is_realtime && d.realtime)
      departure.m_realtime = ParseLocalTime(*d.realtime);
    auto const & track = d.realtime_platform ? d.realtime_platform : d.scheduled_platform;
    if (track)
      departure.m_platform = track->designation;
    departure.m_canceled = d.canceled;
  }
  std::ranges::stable_sort(departures.m_departures, {}, &Departure::GetTime);
  return true;
}
}  // namespace trafiklab
}  // namespace departures
