#include <userver/logging/timestamp.hpp>

#include <optional>

#include <fmt/chrono.h>
#include <fmt/compile.h>
#include <fmt/format.h>

#include <userver/compiler/thread_local.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/strerror.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace {

using TimePoint = std::chrono::system_clock::time_point;
using SecondsTimePoint = std::chrono::time_point<TimePoint::clock, std::chrono::seconds>;

struct CachedTime final {
    std::optional<SecondsTimePoint> time;
    TimeString string{};
};

template <std::tm* (*TimeConverter)(const std::time_t*, std::tm*)>
TimeString GetCurrentTimeString(TimePoint time, compiler::ThreadLocalScope<CachedTime>& cached_time) noexcept {
    const auto rounded_time = std::chrono::time_point_cast<std::chrono::seconds>(time);
    if (rounded_time != cached_time->time) {
        std::tm tm{};
        std::time_t time_t = std::chrono::system_clock::to_time_t(time);
        if (TimeConverter(&time_t, &tm) != nullptr) {
            fmt::format_to(cached_time->string.data, FMT_COMPILE("{:%FT%T}"), tm);
            cached_time->time = rounded_time;
        } else {
            UASSERT_MSG(false, utils::strerror(errno));

            // ... keep using the old cached time
        }
    }
    return cached_time->string;
}

compiler::ThreadLocal thread_local_cached_gmt_time = [] { return CachedTime{}; };
compiler::ThreadLocal thread_local_cached_local_time = [] { return CachedTime{}; };

}  // namespace

TimeString GetCurrentGMTimeString(std::chrono::system_clock::time_point time) noexcept {
    auto cached_time = thread_local_cached_gmt_time.Use();
    return GetCurrentTimeString<gmtime_r>(time, cached_time);
}

TimeString GetCurrentLocalTimeString(std::chrono::system_clock::time_point time) noexcept {
    auto cached_time = thread_local_cached_local_time.Use();
    return GetCurrentTimeString<localtime_r>(time, cached_time);
}

namespace impl {

std::chrono::microseconds::rep FractionalMicroseconds(std::chrono::system_clock::time_point time) noexcept {
    return std::chrono::time_point_cast<std::chrono::microseconds>(time).time_since_epoch().count() % 1'000'000;
}

}  // namespace impl

}  // namespace logging

USERVER_NAMESPACE_END
