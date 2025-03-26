#include <logging/timestamp.hpp>

#include <fmt/chrono.h>
#include <fmt/compile.h>
#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace logging {

unsigned long FractionalMicroseconds(TimePoint time) noexcept {
    return std::chrono::time_point_cast<std::chrono::microseconds>(time).time_since_epoch().count() % 1'000'000;
}

compiler::ThreadLocal local_cached_time = [] { return CachedTime{}; };

template <std::tm (*TimeConverter)(std::time_t)>
TimeString GetCurrentTimeString(TimePoint now) noexcept {
    auto cached_time = local_cached_time.Use();

    const auto rounded_now = std::chrono::time_point_cast<std::chrono::seconds>(now);
    if (rounded_now != cached_time->time) {
        fmt::format_to(
            cached_time->string.data, FMT_COMPILE("{:%FT%T}"), TimeConverter(std::chrono::system_clock::to_time_t(now))
        );
        cached_time->time = rounded_now;
    }
    return cached_time->string;
}

TimeString GetCurrentGMTimeString(TimePoint now) noexcept { return GetCurrentTimeString<fmt::gmtime>(now); }

TimeString GetCurrentLocalTimeString(TimePoint now) noexcept { return GetCurrentTimeString<fmt::localtime>(now); }

}  // namespace logging

USERVER_NAMESPACE_END
