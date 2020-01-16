#pragma once

/// @file hostinfo/blocking/get_hostname.hpp
/// @brief @copybrief hostinfo::blocking::GetRealHostName

#include <string>

namespace hostinfo::blocking {

/// @brief Returns host name.
/// @warning This is a blocking function.
std::string GetRealHostName();

}  // namespace hostinfo::blocking
