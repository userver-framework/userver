#pragma once

/// @file userver/utils/mock_now.hpp
/// @brief Mocking and getting mocked time values
/// @ingroup userver_universal

#include <chrono>

#include <userver/utils/datetime/wall_coarse_clock.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime {

/// @cond
std::chrono::system_clock::time_point MockNow() noexcept;

std::chrono::steady_clock::time_point MockSteadyNow() noexcept;

WallCoarseClock::time_point MockWallCoarseNow() noexcept;
///@endcond

/// Sets the mocked value for utils::datetime::Now(), utils::datetime::SteadyNow() and
/// utils::datetime::WallCoarseClock::Now().
///
/// Thread safe.
void MockNowSet(std::chrono::system_clock::time_point new_mocked_now);

/// Adds duration to current mocked time point value
///
/// Thread safe.
///
/// @throws utils::InvariantError if IsMockNow() returns false.
void MockSleep(std::chrono::milliseconds duration);

/// Removes time point mocking for utils::datetime::Now(), utils::datetime::SteadyNow() and
/// utils::datetime::WallCoarseClock::Now().
void MockNowUnset() noexcept;

/// Returns true if time point is mocked for utils::datetime::Now(), utils::datetime::SteadyNow() and
/// utils::datetime::WallCoarseClock::Now().
bool IsMockNow() noexcept;

}  // namespace utils::datetime

USERVER_NAMESPACE_END
