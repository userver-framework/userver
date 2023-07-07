#pragma once

/// @file userver/hostinfo/blocking/read_groups.hpp
/// @brief @copybrief hostinfo::blocking::ReadConductorGroups

#include <string>
#include <vector>

USERVER_NAMESPACE_BEGIN

/// Blocking functions for getting information about hosts
namespace hostinfo::blocking {

/// @brief Reads Conductor groups from conductor-hostinfo file.
/// @throw `std::runtime_error` if file cannot be read.
/// @warning This is a blocking function.
std::vector<std::string> ReadConductorGroups();

}  // namespace hostinfo::blocking

USERVER_NAMESPACE_END
