#pragma once

/// @file formats/common/path.hpp
/// @brief @copybrief formats::common::Path

#include <string>
#include <vector>

namespace formats::common {

/// Document/array element path storage
using Path = std::vector<std::string>;

/// @brief Converts provided Path into a human-readable representation
/// @param path path to convert
/// @param root empty path representation
/// @param separator path tokens separator
std::string PathToString(const Path& path, const std::string& root = "/",
                         char separator = '.');

}  // namespace formats::common
