#pragma once

/// @file userver/hostinfo/blocking/get_hostname.hpp
/// @brief @copybrief hostinfo::blocking::GetRealHostName

#include <string>

USERVER_NAMESPACE_BEGIN

namespace hostinfo::blocking {

/// @brief Returns host name.
/// @warning This is a blocking function.
std::string GetRealHostName();

}  // namespace hostinfo::blocking

USERVER_NAMESPACE_END
