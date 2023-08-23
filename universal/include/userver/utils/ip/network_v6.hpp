#pragma once

/// @file userver/utils/ip/network_v6.hpp
/// @brief @copybrief utils::ip::NetworkV6

#include <userver/utils/ip/address_v6.hpp>
#include <userver/utils/ip/network_base.hpp>

USERVER_NAMESPACE_BEGIN

// IP address and utilities
namespace utils::ip {

/// @ingroup userver_containers
///
/// @brief IPv6 network.
using NetworkV6 = detail::NetworkBase<AddressV6>;

/// @brief Get the network as an address in dotted decimal format.
std::string NetworkV6ToString(const NetworkV6& network);

/// @brief Create an IPv6 network from a string containing IP address and prefix
/// length.
NetworkV6 NetworkV6FromString(const std::string& str);

/// @brief Convert NetworkV4 to CIDR format
NetworkV6 TransformToCidrFormat(NetworkV6 network);

}  // namespace utils::ip

USERVER_NAMESPACE_END
