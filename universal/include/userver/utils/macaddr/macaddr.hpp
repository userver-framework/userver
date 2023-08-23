#pragma once

/// @file userver/utils/macaddr/macaddr.hpp
/// @brief @copybrief utils::datetime::Macaddr

#include <array>
#include <string>

#include <userver/utils/macaddr/macaddr_base.hpp>

USERVER_NAMESPACE_BEGIN

/// Macaddr and utilities
namespace utils::macaddr {

using Macaddr = MacaddrBase<6>;

/// @brief Get the address as a string in "xx:xx:.." format.
std::string MacaddrToString(Macaddr macaddr);

/// @brief Get the MAC-address from std::string.
Macaddr MacaddrFromString(const std::string& str);

}  // namespace utils::macaddr

USERVER_NAMESPACE_END
