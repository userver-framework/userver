#pragma once

/// @file userver/utils/statistics/recentperiod_detail.hpp
/// @brief RecentPeriod template type traits and concepts

USERVER_NAMESPACE_BEGIN

namespace utils::statistics::detail {

// Detect if the result type provides Add(Counter, Duration, Duration) function
template <typename Result, typename Counter, typename Duration>
concept ResultWantsAddFunction = requires(Result& r, Counter c, Duration d) { r.Add(c, d, d); };

// Detect if a counter can be added to the result
template <typename Result, typename Counter>
concept ResultCanUseAddAssign = requires(Result& r, const Counter& c) { r += c; };

// Detect if a Counter provides a Reset function
template <typename Counter>
concept CanReset = requires(Counter& c) { c.Reset(); };

}  // namespace utils::statistics::detail

USERVER_NAMESPACE_END
