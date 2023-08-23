#include "address_utils.hpp"

#include <userver/utils/ip/address_v4.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::ip {

/// @brief Create an IPv4 address from an IP address string in dotted decimal
/// form.
AddressV4 AddressV4FromString(const std::string& str) {
  return detail::AddressFromString<4>(str);
}

/// @brief Get the address as a string in dotted decimal format.
std::string AddressV4ToString(const AddressV4& address) {
  return detail::AddressToString(address);
}

}  // namespace utils::ip

USERVER_NAMESPACE_END
