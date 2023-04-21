#pragma once

/// @file userver/utils/statistics/pretty_format.hpp
/// @brief Statistics output in human-readable format.

#include <string>

#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

// clang-format off

// Output `statistics` in format:
// `"metric-path: some-label=label-value, some-other-label=other-label-value\t42`
std::string ToPrettyFormat(
    const utils::statistics::Storage& statistics,
    const utils::statistics::Request& statistics_request = {});

// clang-format on

}  // namespace utils::statistics

USERVER_NAMESPACE_END
