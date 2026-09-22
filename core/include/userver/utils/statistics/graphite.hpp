#pragma once

/// @file userver/utils/statistics/graphite.hpp
/// @brief Statistics output in Graphite format.

#include <string>

#include <userver/utils/statistics/request.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/// Output `statistics` in Graphite format with tags (labels).
std::string ToGraphiteFormat(const Storage& statistics, const Request& statistics_request = {});

std::string ToGraphiteFormat(WriterFuncRef writer, const Request& statistics_request = {});

}  // namespace utils::statistics

USERVER_NAMESPACE_END
