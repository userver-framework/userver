#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>

#include <userver/utils/datetime/steady_coarse_clock.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace impl {

/// Some stats about thread CPU usage, consult getrusage(2) for more context.
struct ThreadCpuUsage final {
  /// User CPU time used since the thread creation.
  std::chrono::microseconds user{};
  /// System CPU time used since the thread creation.
  std::chrono::microseconds system{};
};

ThreadCpuUsage GetCurrentThreadCpuUsage();

}  // namespace impl

/// @brief Helper class to maintain current thread CPU usage.
/// Doesn't drive itself, so you are expected to call `Collect` every so often.
/// Getting current thread CPU usage implies syscall, the class throttles it via
/// `collect_interval` and `throttle`: every `throttle` calls if
/// `collect_interval` has passed since last syscall query OS again.
/// @note `Collect` is not thread-safe, as it doesn't make sense to mix stats
/// from different threads. `GetCurrentLoadPercent` is thread-safe.
class ThreadCpuStatsStorage final {
 public:
  /// @param collect_interval query OS not often than provided interval
  /// @param throttle throttle querying OS with this value
  ThreadCpuStatsStorage(std::chrono::milliseconds collect_interval,
                        std::size_t throttle);

  /// @brief Collect current CPU usage (might get throttled away).
  void Collect();

  /// @brief Get current CPU usage percent.
  std::uint8_t GetCurrentLoadPercent() const;

 private:
  using Clock = utils::datetime::SteadyCoarseClock;

  bool ValidateThreadSafety();
  bool Throttle();
  void DoCollect();

  std::chrono::milliseconds collect_interval_;
  std::size_t throttle_;

  std::size_t times_called_{0};
  Clock::time_point last_ts_{};
  utils::statistics::impl::ThreadCpuUsage last_usage_{};

  std::atomic<std::uint8_t> current_usage_pct_{0};

  std::thread::id caller_id_;
};

}  // namespace utils::statistics

USERVER_NAMESPACE_END
