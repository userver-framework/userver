#pragma once

/// @file userver/utils/statistics/prometheus.hpp
/// @brief @copybrief utils::statistics::ToPrometheusFormat

#include <string>
#include <unordered_map>

#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace impl {

// The metric name specifies the general feature of a system that is measured
// (e.g. http_requests_total - the total number of HTTP requests received).
// It may contain ASCII letters and digits, as well as underscores and colons.
// It must match the regex [a-zA-Z_][a-zA-Z0-9_]*.
std::string ToPrometheusName(std::string_view data);

// Label names may contain ASCII letters, numbers, as well as underscores.
// They must match the regex [a-zA-Z_][a-zA-Z0-9_]*.
// Label names beginning with __ are reserved for internal use.
std::string ToPrometheusLabel(std::string_view name);

}  // namespace impl

/// Output `statistics` in Prometheus format
std::string ToPrometheusFormat(
    const std::unordered_map<std::string, std::string>& common_labels,
    const utils::statistics::Storage& statistics,
    const utils::statistics::StatisticsRequest& statistics_request = {});

}  // namespace utils::statistics

USERVER_NAMESPACE_END
