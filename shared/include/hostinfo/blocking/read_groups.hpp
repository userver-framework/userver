#pragma once

/// @file hostinfo/blocking/read_groups.hpp
/// @brief @copybrief hostinfo::blocking::ReadConductorGroups

#include <string>
#include <vector>

namespace hostinfo::blocking {

/// @brief Reads Conductor groups from conductor-hostinfo file.
/// @throw `std::runtime_error` if file cannot be read.
/// @warn This is a blocking function.
std::vector<std::string> ReadConductorGroups();

}  // namespace hostinfo::blocking
