#include "address_utils.hpp"

#include <userver/utils/ip/address_v6.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::ip {

std::string AddressV6ToString(const AddressV6& address) {
  return detail::AddressToString(address);
}

AddressV6 AddressV6FromString(const std::string& str) {
  return detail::AddressFromString<16>(str);
}

}  // namespace utils::ip

USERVER_NAMESPACE_END
