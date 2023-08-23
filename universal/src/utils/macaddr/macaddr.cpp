#include "macaddr_utils.hpp"

#include <fmt/printf.h>

#include <userver/utils/macaddr/macaddr.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::macaddr {

Macaddr MacaddrFromString(const std::string& str) {
  const auto [octets, octets_counter] =
      detail::MacaddrOctetsFromString<Macaddr::OctetsType().size()>(str);
  if (octets_counter != octets.size()) {
    throw std::invalid_argument(std::to_string(octets_counter));
  }
  return Macaddr(octets);
}

std::string MacaddrToString(Macaddr macaddr) {
  const auto octets = macaddr.GetOctets();
  return fmt::sprintf("%02x:%02x:%02x:%02x:%02x:%02x", octets[0], octets[1],
                      octets[2], octets[3], octets[4], octets[5]);
}
}  // namespace utils::macaddr

USERVER_NAMESPACE_END
