#pragma once

#ifdef MOCK_NOW
#include <chrono>

namespace utils::datetime {

// Note: all mock_now.hpp methods are thread-safe

std::chrono::system_clock::time_point MockNow();
std::chrono::steady_clock::time_point MockSteadyNow();
void MockNowSet(std::chrono::system_clock::time_point new_mocked_now);
void MockSleep(std::chrono::seconds duration);
void MockSleep(std::chrono::milliseconds duration);
void MockNowUnset();
bool IsMockNow();

}  // namespace utils::datetime

#endif  // MOCK_NOW
