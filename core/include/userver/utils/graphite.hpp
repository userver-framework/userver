#pragma once

/// @file userver/utils/graphite.hpp
/// @brief @copybrief utils::graphite::EscapeName

#include <string>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace utils::graphite {

/// @brief Escapes a string for use as a Graphite metric name
std::string EscapeName(std::string_view s);

}  // namespace utils::graphite

USERVER_NAMESPACE_END
