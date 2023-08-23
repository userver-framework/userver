#include "network_utils.hpp"

#include <userver/utils/ip/network_v4.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::ip {

std::string NetworkV4ToString(const NetworkV4& network) {
  return fmt::format("{}/{}", AddressV4ToString(network.GetAddress()),
                     network.GetPrefixLength());
}

NetworkV4 NetworkV4FromString(const std::string& str) {
  return detail::NetworkFromString<AddressV4>(str);
}

NetworkV4 TransformToCidrFormat(NetworkV4 network) {
  return detail::TransformToCidrNetwork<AddressV4>(network);
}

}  // namespace utils::ip

USERVER_NAMESPACE_END
