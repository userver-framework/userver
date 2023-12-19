#pragma once

#include <chrono>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

void SetLogLimitedEnable(bool enable) noexcept;

bool IsLogLimitedEnabled() noexcept;

void SetLogLimitedInterval(std::chrono::steady_clock::duration d) noexcept;

std::chrono::steady_clock::duration GetLogLimitedInterval() noexcept;

}  // namespace logging::impl

USERVER_NAMESPACE_END
