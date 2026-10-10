#include "departures/departures.hpp"

#include <charconv>

namespace departures
{
namespace
{
bool ParseNumber(std::string_view s, size_t pos, size_t size, int & value)
{
  if (pos + size > s.size())
    return false;
  auto const * begin = s.data() + pos;
  auto const * end = begin + size;
  auto const [ptr, ec] = std::from_chars(begin, end, value);
  return ec == std::errc() && ptr == end;
}
}  // namespace

std::optional<LocalTime> ParseLocalTime(std::string_view s)
{
  using namespace std::chrono;

  int y, mo, d, h, mi, sec = 0;
  if (!ParseNumber(s, 0, 4, y) || s.size() < 16 || s[4] != '-' || !ParseNumber(s, 5, 2, mo) || s[7] != '-' ||
      !ParseNumber(s, 8, 2, d) || (s[10] != 'T' && s[10] != ' ') || !ParseNumber(s, 11, 2, h) || s[13] != ':' ||
      !ParseNumber(s, 14, 2, mi))
  {
    return {};
  }
  if (s.size() > 16 && s[16] == ':' && !ParseNumber(s, 17, 2, sec))
    return {};

  year_month_day const date{year{y}, month{static_cast<unsigned>(mo)}, day{static_cast<unsigned>(d)}};
  if (!date.ok() || h > 23 || mi > 59 || sec > 60)
    return {};
  return local_days{date} + hours{h} + minutes{mi} + seconds{sec};
}

std::string DebugPrint(TransportMode mode)
{
  switch (mode)
  {
  case TransportMode::Bus: return "Bus";
  case TransportMode::Tram: return "Tram";
  case TransportMode::Metro: return "Metro";
  case TransportMode::Train: return "Train";
  case TransportMode::Ferry: return "Ferry";
  case TransportMode::Other: return "Other";
  }
  return "Unknown";
}

std::string DebugPrint(Status status)
{
  switch (status)
  {
  case Status::Ok: return "Ok";
  case Status::StopNotFound: return "StopNotFound";
  case Status::NetworkError: return "NetworkError";
  case Status::Unauthorized: return "Unauthorized";
  case Status::ServerError: return "ServerError";
  }
  return "Unknown";
}
}  // namespace departures
