#pragma once

/// @file userver/utils/statistics/pretty_format.hpp
/// @brief Statistics output in human-readable format.

#include <string>

#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/// @brief Output `statistics` in a human-readable, compact format, useful for
/// visual inspection.
///
/// @warning This format is unstable, we reserve the right to change it at any
/// point. Don't use it in scripts, use utils::statistics::ToJsonFormat instead!
///
/// The current version of this unstable format is:
/// @code
/// metric-path: some-label=label-value, other-label=other-label-value\tRATE\t42
/// @endcode
std::string ToPrettyFormat(
    const utils::statistics::Storage& statistics,
    const utils::statistics::Request& statistics_request = {});

}  // namespace utils::statistics

USERVER_NAMESPACE_END
