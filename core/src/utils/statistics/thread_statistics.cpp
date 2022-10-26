#include <utils/statistics/thread_statistics.hpp>

#include <sys/resource.h>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace impl {

ThreadCpuUsage GetCurrentThreadCpuUsage() {
#ifdef RUSAGE_THREAD
  rusage usage{};
  if (getrusage(RUSAGE_THREAD, &usage) != 0) {
    // Shouldn't really happen
    return {};
  }

  const auto timeval_to_mcs = [](timeval tv) {
    return std::chrono::microseconds{
        static_cast<std::uint64_t>(tv.tv_sec) * 1'000'000 + tv.tv_usec};
  };

  ThreadCpuUsage result;
  result.user = timeval_to_mcs(usage.ru_utime);
  result.system = timeval_to_mcs(usage.ru_stime);
  return result;
#else
  return {};
#endif
}

}  // namespace impl

ThreadCpuStatsStorage::ThreadCpuStatsStorage(
    std::chrono::milliseconds collect_interval, std::size_t throttle)
    : collect_interval_{collect_interval}, throttle_{throttle} {}

void ThreadCpuStatsStorage::Collect() {
  ValidateThreadSafety();

  if (Throttle()) {
    DoCollect();
  }
}

std::uint8_t ThreadCpuStatsStorage::GetCurrentLoadPct() const {
  return current_usage_pct_.load(std::memory_order_relaxed);
}

bool ThreadCpuStatsStorage::Throttle() {
  ++times_called_;
  if (times_called_ < throttle_) {
    return false;
  }
  times_called_ -= throttle_;

  return true;
}

void ThreadCpuStatsStorage::DoCollect() {
  const auto now = Clock::now();
  if (last_ts_ != Clock::time_point{} && (now - last_ts_ < collect_interval_)) {
    return;
  }

  const auto usage = impl::GetCurrentThreadCpuUsage();
  if (last_ts_ != Clock::time_point{}) {
    const auto usage_pct = std::min(
        100L,
        (usage.user + usage.system - last_usage_.user - last_usage_.system) *
            100 / (now - last_ts_));
    current_usage_pct_.store(static_cast<std::uint8_t>(usage_pct),
                             std::memory_order_relaxed);
  }

  last_usage_ = usage;
  last_ts_ = now;
}

void ThreadCpuStatsStorage::ValidateThreadSafety() {
#ifndef NDEBUG
  const auto thread_id = std::this_thread::get_id();
  if (!caller_id_.has_value()) {
    caller_id_.emplace(thread_id);
  } else {
    UASSERT_MSG(
        *caller_id_ == thread_id,
        "ThreadCpuStatsStorage is not thread-safe, as it doesn't make sense.");
  }
#endif
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
