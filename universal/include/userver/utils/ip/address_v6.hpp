#pragma once

/// @file userver/utils/ip/address_v6.hpp
/// @brief @copybrief utils::ip::AddressV6

#include <string>

#include <userver/utils/ip/address_base.hpp>

USERVER_NAMESPACE_BEGIN

// IP address and utilities
namespace utils::ip {

/// @ingroup userver_containers
///
/// @brief IPv6 address in network bytes order
using AddressV6 = detail::AddressBase<16>;

/// @brief Get the address as a string in dotted decimal format.
std::string AddressV6ToString(const AddressV6& address);

/// @brief Create an IPv6 address from an IP address string in dotted decimal
/// form.
AddressV6 AddressV6FromString(const std::string& str);

}  // namespace utils::ip

USERVER_NAMESPACE_END
