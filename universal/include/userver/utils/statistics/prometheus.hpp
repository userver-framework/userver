#pragma once

/// @file userver/utils/statistics/prometheus.hpp
/// @brief Statistics output in Prometheus format.
/// @ingroup userver_universal

#include <string>
#include <string_view>

#include <userver/utils/statistics/request.hpp>
#include <userver/utils/statistics/visit.hpp>

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

/// Output `statistics` in Prometheus format, each metric has `gauge` type.
std::string ToPrometheusFormat(WriterFuncRef writer, const Request& request = {});

/// @overload
std::string ToPrometheusFormat(const HasWriterSupport auto& metric, const Request& request = {}) {
    return ToPrometheusFormat([&metric](Writer& writer) { writer = metric; }, request);
}

/// Output `statistics` in Prometheus format, without metric types.
std::string ToPrometheusFormatUntyped(WriterFuncRef writer, const Request& request = {});

/// @overload
std::string ToPrometheusFormatUntyped(const HasWriterSupport auto& metric, const Request& request = {}) {
    return ToPrometheusFormatUntyped([&metric](Writer& writer) { writer = metric; }, request);
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
