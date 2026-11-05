#pragma once

/// @file userver/utils/statistics/graphite.hpp
/// @brief Statistics output in Graphite format.

#include <string>

#include <userver/utils/statistics/request.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/// Output `statistics` in Graphite format with tags (labels).
std::string ToGraphiteFormat(WriterFuncRef writer, const Request& statistics_request = {});

/// @overload
std::string ToGraphiteFormat(const HasWriterSupport auto& metric, const Request& statistics_request = {}) {
    return ToGraphiteFormat([&metric](Writer& writer) { writer = metric; }, statistics_request);
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
