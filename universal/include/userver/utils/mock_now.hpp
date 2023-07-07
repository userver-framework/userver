#pragma once

/// @file userver/utils/mock_now.hpp
/// @brief Mocking and getting mocked values

#include <chrono>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime {

// Note: all mock_now.hpp methods are thread-safe

std::chrono::system_clock::time_point MockNow() noexcept;
std::chrono::steady_clock::time_point MockSteadyNow() noexcept;
void MockNowSet(std::chrono::system_clock::time_point new_mocked_now);
void MockSleep(std::chrono::seconds duration);
void MockSleep(std::chrono::milliseconds duration);
void MockNowUnset();
bool IsMockNow();

}  // namespace utils::datetime

USERVER_NAMESPACE_END
