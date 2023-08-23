#pragma once

/// @file userver/utils/ip/network_v4.hpp
/// @brief @copybrief utils::ip::NetworkV4

#include <userver/utils/ip/address_v4.hpp>
#include <userver/utils/ip/network_base.hpp>

USERVER_NAMESPACE_BEGIN

// IP address and utilities
namespace utils::ip {

/// @ingroup userver_containers
///
/// @brief IPv4 network.
using NetworkV4 = detail::NetworkBase<AddressV4>;

///@brief Get the network as an address in dotted decimal format.
std::string NetworkV4ToString(const NetworkV4& network);

///@brief Create an IPv4 network from a string containing IP address and prefix
/// length.
/// @throw std::invalid_argument, AddressSystemError 
NetworkV4 NetworkV4FromString(const std::string& str);

/// @brief Convert NetworkV4 to CIDR format
NetworkV4 TransformToCidrFormat(NetworkV4 network);

}  // namespace utils::ip

USERVER_NAMESPACE_END
