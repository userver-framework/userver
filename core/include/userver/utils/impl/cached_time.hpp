#pragma once

#include <chrono>
#include <tuple>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

using SystemTimePoint = std::chrono::system_clock::time_point;
using SteadyTimePoint = std::chrono::steady_clock::time_point;

// To serialize a steady_clock::time_point, we need to convert it to a
// system_clock::time_point using now(). Calling now() myriad times in a row may
// be costly, so as an optimization we cache now() globally.
void UpdateGlobalTime();

// Note: the two time points are not synchronized. They may also come from
// different 'UpdateGlobalTime' calls.
std::tuple<SystemTimePoint, SteadyTimePoint> GetGlobalTime();

}  // namespace utils::impl

USERVER_NAMESPACE_END
