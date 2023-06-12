#include <utils/statistics/thread_statistics.hpp>

#include <algorithm>

#include <sys/resource.h>  // for RUSAGE_THREAD

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

  const auto timeval_to_mcs = [](const timeval& tv) {
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
    : collect_interval_{collect_interval}, throttle_{throttle} {
  UASSERT(collect_interval_.count() > 0);
  UASSERT(throttle);
}

void ThreadCpuStatsStorage::Collect() {
  UASSERT_MSG(
      ValidateThreadSafety(),
      "ThreadCpuStatsStorage is not thread-safe, as it doesn't make sense.");

  if (Throttle()) {
    DoCollect();
  }
}

std::uint8_t ThreadCpuStatsStorage::GetCurrentLoadPercent() const {
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
  if (last_ts_ != Clock::time_point{} &&
      (now - last_ts_ <= collect_interval_)) {
    return;
  }

  const auto usage = impl::GetCurrentThreadCpuUsage();
  if (last_ts_ != Clock::time_point{}) {
    const auto usage_pct =
        std::min(100L,
                 // shouldn't be necessary, but won't hurt
                 std::max<long>(0L, (usage.user + usage.system -
                                     last_usage_.user - last_usage_.system) *
                                        100 / (now - last_ts_)));
    current_usage_pct_.store(static_cast<std::uint8_t>(usage_pct),
                             std::memory_order_relaxed);
  }

  last_usage_ = usage;
  last_ts_ = now;
}

bool ThreadCpuStatsStorage::ValidateThreadSafety() {
  const auto thread_id = std::this_thread::get_id();
  if (caller_id_ == std::thread::id()) {
    caller_id_ = thread_id;
    return true;
  } else {
    return caller_id_ == thread_id;
  }
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
