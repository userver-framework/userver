#pragma once

/// @file userver/utils/statistics/graphite.hpp
/// @brief Statistics output in Graphite format.

#include <string>
#include <unordered_map>

#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/// Output `statistics` in Graphite format with tags (labels).
///
/// @note `common_labels` should not match labels from `statistics`.
std::string ToGraphiteFormat(
    const std::unordered_map<std::string, std::string>& common_labels,
    const utils::statistics::Storage& statistics,
    const utils::statistics::StatisticsRequest& statistics_request = {});

}  // namespace utils::statistics

USERVER_NAMESPACE_END
