#pragma once

#include <chrono>
#include <string_view>

#include <userver/compiler/thread_local.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

using TimePoint = std::chrono::system_clock::time_point;

unsigned long FractionalMicroseconds(TimePoint time) noexcept;

using SecondsTimePoint = std::chrono::time_point<TimePoint::clock, std::chrono::seconds>;
constexpr std::string_view kTimeTemplate = "0000-00-00T00:00:00";

struct TimeString final {
    char data[kTimeTemplate.size()]{};

    std::string_view ToStringView() const noexcept { return {data, std::size(data)}; }
};

struct CachedTime final {
    SecondsTimePoint time{};
    TimeString string{};
};

TimeString GetCurrentGMTimeString(TimePoint now) noexcept;
TimeString GetCurrentLocalTimeString(TimePoint now) noexcept;

}  // namespace logging

USERVER_NAMESPACE_END
