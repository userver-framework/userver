#pragma once

/// @file userver/logging/timestamp.hpp
/// @brief Coroutine-safe date and time formatting utilities
/// @ingroup userver_universal

#include <chrono>
#include <cstddef>
#include <string_view>

#include <userver/compiler/impl/lifetime.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

/// A fixed-size date and time string in `YYYY-MM-DDTHH:MM:SS` format.
///
/// The contents are not null-terminated. Use ToStringView() to access them.
struct TimeString final {
    static constexpr std::size_t kSize = sizeof("0000-00-00T00:00:00") - 1;

    char data[kSize]{};

    std::string_view ToStringView() const noexcept USERVER_IMPL_LIFETIME_BOUND { return {data, sizeof(data)}; }
};

/// @brief Formats @p time in UTC as `YYYY-MM-DDTHH:MM:SS`.
///
/// This function is coroutine-safe and does not block. It caches the formatted
/// value in coroutine-safe thread-local storage.
TimeString GetCurrentGMTimeString(std::chrono::system_clock::time_point time) noexcept;

/// @brief Formats @p time in the system local timezone as `YYYY-MM-DDTHH:MM:SS`.
///
/// This function is coroutine-safe and does not block. It caches the formatted
/// value in coroutine-safe thread-local storage.
TimeString GetCurrentLocalTimeString(std::chrono::system_clock::time_point time) noexcept;

namespace impl {

std::chrono::microseconds::rep FractionalMicroseconds(std::chrono::system_clock::time_point time) noexcept;

}  // namespace impl

}  // namespace logging

USERVER_NAMESPACE_END
