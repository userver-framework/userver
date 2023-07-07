#pragma once

/// @file userver/utils/datetime/wall_coarse_clock.hpp
/// @brief @copybrief utils::datetime::WallCoarseClock

#include <chrono>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime {

/// @brief System clock with up to a few millisecond resolution that is slightly
/// faster than the std::chrono::system_clock
struct WallCoarseClock final {
  // Duration matches system clock, but it is updated once in a few milliseconds
  using duration = std::chrono::system_clock::duration;
  using rep = duration::rep;
  using period = duration::period;
  using time_point = std::chrono::system_clock::time_point;

  static constexpr bool is_steady = false;

  static time_point now() noexcept;
  static duration resolution() noexcept;
};

}  // namespace utils::datetime

USERVER_NAMESPACE_END
