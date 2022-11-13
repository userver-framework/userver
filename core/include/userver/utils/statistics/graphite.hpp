#pragma once

/// @file userver/utils/statistics/graphite.hpp
/// @brief Statistics output in Graphite format.

#include <string>

#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/// Output `statistics` in Graphite format with tags (labels).
std::string ToGraphiteFormat(
    const utils::statistics::Storage& statistics,
    const utils::statistics::Request& statistics_request = {});

}  // namespace utils::statistics

USERVER_NAMESPACE_END
