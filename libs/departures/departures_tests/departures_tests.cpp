#include "testing/testing.hpp"

#include "departures/departures.hpp"

namespace departures_tests
{
using namespace departures;
using namespace std::chrono;

UNIT_TEST(ParseLocalTime_Formats)
{
  auto const expected = local_days{2025y / April / 1} + 16h + 5min;
  TEST(ParseLocalTime("2025-04-01T16:05") == expected, ());
  TEST(ParseLocalTime("2025-04-01T16:05:00") == expected, ());
  TEST(ParseLocalTime("2025-04-01 16:05:30") == expected + 30s, ());
  // A fraction and a time zone are ignored: departures are compared with the provider's own clock.
  TEST(ParseLocalTime("2025-04-01T16:05:30.123+02:00") == expected + 30s, ());
  TEST(ParseLocalTime("2025-04-01T16:05Z") == expected, ());
}

UNIT_TEST(ParseLocalTime_Invalid)
{
  TEST(!ParseLocalTime(""), ());
  TEST(!ParseLocalTime("2025-04-01"), ());
  TEST(!ParseLocalTime("2025-04-01T16"), ());
  TEST(!ParseLocalTime("2025-13-01T16:05"), ());
  TEST(!ParseLocalTime("2025-02-30T16:05"), ());
  TEST(!ParseLocalTime("2025-04-01T24:00"), ());
  TEST(!ParseLocalTime("2025/04/01T16:05"), ());
  TEST(!ParseLocalTime("2025-04-01T16:05:x0"), ());
}
}  // namespace departures_tests
