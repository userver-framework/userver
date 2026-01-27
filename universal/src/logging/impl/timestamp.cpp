#include <userver/logging/impl/timestamp.hpp>

#include <fmt/chrono.h>
#include <fmt/compile.h>
#include <fmt/format.h>

#include <userver/compiler/thread_local.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/strerror.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

namespace {

using TimePoint = std::chrono::system_clock::time_point;
using SecondsTimePoint = std::chrono::time_point<TimePoint::clock, std::chrono::seconds>;

struct CachedTime final {
    SecondsTimePoint time{};
    TimeString string{};
};

template <std::tm* (*TimeConverter)(const std::time_t*, std::tm*)>
TimeString GetCurrentTimeString(TimePoint now, compiler::ThreadLocalScope<CachedTime>& cached_time) noexcept {
    const auto rounded_now = std::chrono::time_point_cast<std::chrono::seconds>(now);
    if (rounded_now != cached_time->time) {
        std::tm tm{};
        std::time_t now_t = std::chrono::system_clock::to_time_t(now);
        if (TimeConverter(&now_t, &tm) != nullptr) {
            fmt::format_to(cached_time->string.data, FMT_COMPILE("{:%FT%T}"), tm);
            cached_time->time = rounded_now;
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

std::chrono::microseconds::rep FractionalMicroseconds(std::chrono::system_clock::time_point time) noexcept {
    return std::chrono::time_point_cast<std::chrono::microseconds>(time).time_since_epoch().count() % 1'000'000;
}

TimeString GetCurrentGMTimeString(std::chrono::system_clock::time_point now) noexcept {
    auto cached_time = thread_local_cached_gmt_time.Use();
    return GetCurrentTimeString<gmtime_r>(now, cached_time);
}

TimeString GetCurrentLocalTimeString(std::chrono::system_clock::time_point now) noexcept {
    auto cached_time = thread_local_cached_local_time.Use();
    return GetCurrentTimeString<localtime_r>(now, cached_time);
}

}  // namespace logging::impl

USERVER_NAMESPACE_END
