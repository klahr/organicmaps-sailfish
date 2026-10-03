#include "platform/localization.hpp"

#include "std/target_os.hpp"

#include <ctime>

namespace platform
{
std::string GetLocalizedBrandName(std::string const & brand)
{
  return brand;
}

// Sailfish OS reads the type names and strings from the shipped twine files, see localization_sailfish.cpp.
#ifndef OMIM_OS_SAILFISH
std::string GetLocalizedTypeName(std::string const & type)
{
  return type;
}

std::string GetLocalizedString(std::string const & key)
{
  return key;
}
#endif

std::string GetCurrencySymbol(std::string const & currencyCode)
{
  return currencyCode;
}

std::string GetLocalizedMyPositionBookmarkName()
{
  std::time_t t = std::time(nullptr);
  char buf[100] = {0};
  (void)std::strftime(buf, sizeof(buf), "%Ec", std::localtime(&t));
  return buf;
}
}  // namespace platform
