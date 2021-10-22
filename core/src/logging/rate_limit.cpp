#include <logging/rate_limit.hpp>

#include <atomic>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

namespace {

using AtomicDuration = std::atomic<std::chrono::steady_clock::duration>;
static_assert(AtomicDuration::is_always_lock_free);

AtomicDuration& AtomicLogLimitedDuration() noexcept {
  static AtomicDuration duration{std::chrono::seconds(1)};
  return duration;
}

std::atomic<bool>& AtomicLogLimited() noexcept {
  static std::atomic<bool> enabled{true};
  return enabled;
}

}  // namespace

void SetLogLimitedEnable(bool enable) noexcept { AtomicLogLimited() = enable; }

bool IsLogLimitedEnabled() noexcept { return AtomicLogLimited().load(); }

void SetLogLimitedInterval(std::chrono::steady_clock::duration d) noexcept {
  AtomicLogLimitedDuration() = d;
}

std::chrono::steady_clock::duration GetLogLimitedInterval() noexcept {
  return AtomicLogLimitedDuration().load();
}

}  // namespace logging::impl

USERVER_NAMESPACE_END
