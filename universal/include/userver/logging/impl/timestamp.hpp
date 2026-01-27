#pragma once

#include <chrono>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

inline constexpr std::string_view kTimeTemplate = "0000-00-00T00:00:00";

struct TimeString final {
    char data[kTimeTemplate.size()]{};

    std::string_view ToStringView() const noexcept { return {data, std::size(data)}; }
};

TimeString GetCurrentGMTimeString(std::chrono::system_clock::time_point now) noexcept;
TimeString GetCurrentLocalTimeString(std::chrono::system_clock::time_point now) noexcept;
std::chrono::microseconds::rep FractionalMicroseconds(std::chrono::system_clock::time_point time) noexcept;

}  // namespace logging::impl

USERVER_NAMESPACE_END
