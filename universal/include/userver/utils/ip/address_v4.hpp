#pragma once

/// @file userver/utils/ip/address_v4.hpp
/// @brief @copybrief utils::ip::AddressV4

#include <string>

#include <userver/utils/ip/address_base.hpp>

USERVER_NAMESPACE_BEGIN

// IP address and utilities
namespace utils::ip {

/// @ingroup userver_containers
///
/// @brief IPv4 address in network bytes order
using AddressV4 = detail::AddressBase<4>;

/// @brief Create an IPv4 address from an IP address string in dotted decimal
/// form.
/// @throw AddressSystemError
AddressV4 AddressV4FromString(const std::string& str);

/// @brief Get the address as a string in dotted decimal format.
std::string AddressV4ToString(const AddressV4& address);

}  // namespace utils::ip

USERVER_NAMESPACE_END
