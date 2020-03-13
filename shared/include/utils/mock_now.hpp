#pragma once

#ifdef MOCK_NOW
#include <chrono>

namespace utils {
namespace datetime {

std::chrono::system_clock::time_point MockNow();
std::chrono::steady_clock::time_point MockSteadyNow();
void MockNowSet(std::chrono::system_clock::time_point point);
void MockSleep(std::chrono::seconds);
void MockSleep(std::chrono::milliseconds);
void MockNowUnset();
bool IsMockNow();

}  // namespace datetime
}  // namespace utils

#endif  // MOCK_NOW
