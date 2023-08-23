#pragma once

/// @file userver/utils/macaddr/macaddr8.hpp
/// @brief @copybrief utils::datetime::Macaddr8

#include <array>
#include <string>

#include <userver/utils/macaddr/macaddr_base.hpp>

USERVER_NAMESPACE_BEGIN

/// Macaddr and utilities
namespace utils::macaddr {

using Macaddr8 = MacaddrBase<8>; 

std::string Macaddr8ToString(Macaddr8 macaddr);

Macaddr8 Macaddr8FromString(const std::string& str);

}  // namespace utils::macaddr

USERVER_NAMESPACE_END
