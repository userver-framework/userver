#pragma once

/// @file userver/utils/graphite.hpp
/// @brief @copybrief utils::graphite::EscapeName

#include <string>

USERVER_NAMESPACE_BEGIN

namespace utils::graphite {

/// @brief Escapes a string for use as a Graphite metric name
std::string EscapeName(const std::string& s);

}  // namespace utils::graphite

USERVER_NAMESPACE_END
