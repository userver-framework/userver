#pragma once

#include <chrono>

namespace logging::impl {

void SetLogLimitedEnable(bool enable) noexcept;

bool IsLogLimitedEnabled() noexcept;

void SetLogLimitedInterval(std::chrono::steady_clock::duration d) noexcept;

std::chrono::steady_clock::duration GetLogLimitedInterval() noexcept;

}  // namespace logging::impl
