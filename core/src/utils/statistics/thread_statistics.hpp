#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#ifndef NDEBUG
#include <optional>
#include <thread>
#endif

#include <userver/utils/datetime/steady_coarse_clock.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace impl {

struct ThreadCpuUsage final {
  std::chrono::microseconds user{};
  std::chrono::microseconds system{};
};

ThreadCpuUsage GetCurrentThreadCpuUsage();

}  // namespace impl

class ThreadCpuStatsStorage final {
 public:
  ThreadCpuStatsStorage(std::chrono::milliseconds collect_interval,
                        std::size_t throttle = 1);

  void Collect();

  std::uint8_t GetCurrentLoadPct() const;

 private:
  using Clock = utils::datetime::SteadyCoarseClock;

  void ValidateThreadSafety();
  bool Throttle();
  void DoCollect();

  std::chrono::milliseconds collect_interval_;
  std::size_t throttle_;

  std::size_t times_called_{0};
  Clock::time_point last_ts_{};
  utils::statistics::impl::ThreadCpuUsage last_usage_{};

  std::atomic<std::uint8_t> current_usage_pct_{0};

#ifndef NDEBUG
  std::optional<std::thread::id> caller_id_;
#endif
};

}  // namespace utils::statistics

USERVER_NAMESPACE_END
