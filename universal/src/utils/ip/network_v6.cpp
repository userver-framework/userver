#include "network_utils.hpp"

#include <userver/utils/ip/network_v6.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::ip {

std::string NetworkV6ToString(const NetworkV6& network) {
  return fmt::format("{}/{}", AddressV6ToString(network.GetAddress()),
                     network.GetPrefixLength());
}

NetworkV6 NetworkV6FromString(const std::string& str) {
  return detail::NetworkFromString<AddressV6>(str);
}

NetworkV6 TransformToCidrFormat(NetworkV6 network) {
  return detail::TransformToCidrNetwork<AddressV6>(network);
}

}  // namespace utils::ip

USERVER_NAMESPACE_END
