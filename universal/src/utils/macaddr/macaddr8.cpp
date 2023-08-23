#include "macaddr_utils.hpp"

#include <fmt/printf.h>

#include <userver/utils/macaddr/macaddr8.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::macaddr {

Macaddr8 Macaddr8FromString(const std::string& str) {
  auto [octets, octets_counter] =
      detail::MacaddrOctetsFromString<Macaddr8::OctetsType().size()>(str);
  if (octets_counter == 6) {
    octets[7] = octets[5];
    octets[6] = octets[4];
    octets[5] = octets[3];
    octets[3] = 0xFF;
    octets[4] = 0xFE;
  } else if (octets_counter != 8) {
    throw std::invalid_argument(
        "Error while converting MAC-address from string");
  }
  return Macaddr8(octets);
}

std::string Macaddr8ToString(Macaddr8 macaddr) {
  const auto octets = macaddr.GetOctets();
  return fmt::sprintf("%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", octets[0],
                      octets[1], octets[2], octets[3], octets[4], octets[5],
                      octets[6], octets[7]);
}

}  // namespace utils::macaddr

USERVER_NAMESPACE_END
