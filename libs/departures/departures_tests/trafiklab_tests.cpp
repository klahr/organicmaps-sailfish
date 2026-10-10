#include "testing/testing.hpp"

#include "departures/trafiklab.hpp"

namespace trafiklab_tests
{
using namespace departures;
using namespace std::chrono;

// Two groups named like the searched stop, one far away.
std::string_view constexpr kStops = R"({
  "timestamp": "2025-04-01T16:00:00",
  "query": {"queryTime": "2025-04-01T16:00:00", "query": "Odenplan"},
  "stop_groups": [
    {"id": "740000001", "name": "Odenplan", "area_type": "META_STOP", "average_daily_stop_times": 10.5,
     "transport_modes": ["BUS"],
     "stops": [{"id": "1", "name": "Odenplan", "lat": 59.0, "lon": 18.0}]},
    {"id": "740021705", "name": "Odenplan", "area_type": "RIKSHALLPLATS", "average_daily_stop_times": 1234.0,
     "transport_modes": ["BUS", "METRO", "TRAIN"],
     "stops": [{"id": "2", "name": "Odenplan", "lat": 59.3429, "lon": 18.0491},
               {"id": "3", "name": "Odenplan", "lat": 59.3431, "lon": 18.0457}]}
  ]
})";

UNIT_TEST(Trafiklab_NearestStopGroup)
{
  TEST_EQUAL(trafiklab::ParseNearestStopGroup(kStops, {59.3430, 18.0480}), "740021705", ());
  TEST_EQUAL(trafiklab::ParseNearestStopGroup(kStops, {59.0001, 18.0}), "740000001", ());
  // Too far from both.
  TEST(!trafiklab::ParseNearestStopGroup(kStops, {59.1700, 18.0}), ());
  TEST(!trafiklab::ParseNearestStopGroup(R"({"stop_groups": []})", {59.0, 18.0}), ());
  TEST(!trafiklab::ParseNearestStopGroup("not json", {59.0, 18.0}), ());
}

std::string_view constexpr kDepartures = R"({
  "timestamp": "2025-04-01T16:00:12",
  "query": {"queryTime": "2025-04-01T16:00:00", "query": "740021705"},
  "stops": [{"id": "740021705", "name": "Odenplan", "lat": 59.343, "lon": 18.048, "transport_modes": [], "alerts": []}],
  "departures": [
    {"scheduled": "2025-04-01T16:10:00", "realtime": "2025-04-01T16:10:00", "delay": 0, "canceled": false,
     "route": {"name": null, "designation": "4", "transport_mode_code": 700, "transport_mode": "BUS",
               "direction": "Radiohuset",
               "origin": {"id": "1", "name": "Gullmarsplan"}, "destination": {"id": "2", "name": "Radiohuset"}},
     "trip": {"trip_id": "1", "start_date": "2025-04-01", "technical_number": 4},
     "agency": {"id": "1", "name": "SL", "operator": "Keolis"},
     "stop": {"id": "3", "name": "Odenplan", "lat": 59.3, "lon": 18.0},
     "scheduled_platform": {"id": "3", "designation": "A"}, "realtime_platform": null,
     "alerts": [], "is_realtime": false},
    {"scheduled": "2025-04-01T16:02:00", "realtime": "2025-04-01T16:04:30", "delay": 150, "canceled": false,
     "route": {"name": "Gröna linjen", "designation": "18", "transport_mode_code": 401, "transport_mode": "METRO",
               "direction": "Farsta strand", "destination": {"id": "4", "name": "Farsta strand"}},
     "scheduled_platform": {"id": "5", "designation": "1"}, "realtime_platform": {"id": "6", "designation": "2"},
     "alerts": [], "is_realtime": true},
    {"scheduled": "2025-04-01T16:06:00", "realtime": "2025-04-01T16:06:00", "delay": 0, "canceled": true,
     "route": {"designation": "41", "transport_mode": "TRAIN", "direction": "Märsta"},
     "alerts": [], "is_realtime": true},
    {"scheduled": "garbage", "route": {"designation": "1", "transport_mode": "BUS", "direction": "X"}}
  ]
})";

UNIT_TEST(Trafiklab_Departures)
{
  Departures result;
  TEST(trafiklab::ParseDepartures(kDepartures, result), ());
  auto const at = [](auto time) { return local_days{2025y / April / 1} + 16h + time; };
  TEST(result.m_now == at(12s), ());
  // Sorted by the expected time; one without a valid time is dropped.
  TEST_EQUAL(result.m_departures.size(), 3, ());

  auto const & metro = result.m_departures[0];
  TEST_EQUAL(metro.m_line, "18", ());
  TEST_EQUAL(metro.m_destination, "Farsta strand", ());
  TEST_EQUAL(metro.m_mode, TransportMode::Metro, ());
  TEST(metro.m_scheduled == at(2min), ());
  TEST(metro.m_realtime == at(4min + 30s), ());
  TEST_EQUAL(metro.m_platform, "2", ());
  TEST(!metro.m_canceled, ());

  auto const & train = result.m_departures[1];
  TEST_EQUAL(train.m_line, "41", ());
  // Without a destination.
  TEST_EQUAL(train.m_destination, "Märsta", ());
  TEST_EQUAL(train.m_mode, TransportMode::Train, ());
  TEST(train.m_canceled, ());
  TEST(train.m_platform.empty(), ());

  auto const & bus = result.m_departures[2];
  TEST_EQUAL(bus.m_line, "4", ());
  TEST_EQUAL(bus.m_destination, "Radiohuset", ());
  TEST_EQUAL(bus.m_mode, TransportMode::Bus, ());
  // A realtime field without a prediction is the timetable.
  TEST(!bus.m_realtime, ());
  TEST(bus.GetTime() == at(10min), ());
  TEST_EQUAL(bus.m_platform, "A", ());
}

UNIT_TEST(Trafiklab_DeparturesInvalid)
{
  Departures result;
  TEST(!trafiklab::ParseDepartures("{}", result), ());
  TEST(!trafiklab::ParseDepartures("[]", result), ());
  TEST(!trafiklab::ParseDepartures(R"({"timestamp": "now", "departures": []})", result), ());
}

UNIT_TEST(Trafiklab_Covers)
{
  TrafiklabProvider const provider("key");
  TEST(provider.Covers({59.33, 18.06}), ("Stockholm"));
  TEST(provider.Covers({67.86, 20.22}), ("Kiruna"));
  TEST(!provider.Covers({52.52, 13.40}), ("Berlin"));
}
}  // namespace trafiklab_tests
