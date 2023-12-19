#include <utils/statistics/thread_statistics.hpp>

#include <algorithm>
#include <utility>

#include <pthread.h>
#include <sys/resource.h>  // for RUSAGE_THREAD

#include <userver/logging/log.hpp>
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

Percent ThreadCpuStatsStorage::GetCurrentLoadPercent() const {
  return current_load_pct_.load(std::memory_order_relaxed);
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
    current_load_pct_.store(static_cast<Percent>(usage_pct),
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

#if !defined(__APPLE__)
class ThreadPoolCpuStatsStorage::Impl final {
 public:
  Impl(const std::vector<std::thread>& threads) {
    threads_.reserve(threads.size());
    for (const auto& thread : threads) {
      clockid_t cid{-1};
      if (pthread_getcpuclockid(
              // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
              const_cast<std::thread&>(thread).native_handle(), &cid)) {
        LOG_ERROR() << "pthread_getcpuclockid call failed unexpectedly, "
                       "something went terribly wrong. CPU load collection "
                       "will be a dummy no-op (always zero)";
        failed_at_initialization_ = true;
      }
      threads_.push_back({cid, std::chrono::microseconds{0}});
    }
  }

  std::vector<Percent> Collect() {
    if (failed_at_initialization_) {
      return std::vector<Percent>(threads_.size(), 0);
    }

    std::vector<Percent> result{};
    result.reserve(threads_.size());

    const auto now = Clock::now();
    const auto time_delta = now - last_ts_;

    for (auto& meta : threads_) {
      const auto current_usage = GetThreadCpuUsage(meta.clock_id);
      const auto last_usage = std::exchange(meta.last_usage, current_usage);

      if (last_ts_ == Clock::time_point{}) {
        result.push_back(0);
      } else {
        const auto current_usage_pct =
            (current_usage - last_usage) * 100 /
            std::chrono::duration_cast<std::chrono::microseconds>(time_delta);
        using CurrentUsageType = decltype(current_usage_pct);

        constexpr CurrentUsageType kMinAllowedUsage{0};
        constexpr CurrentUsageType kMaxAllowedUsage{100};

        result.push_back(
            std::clamp(current_usage_pct, kMinAllowedUsage, kMaxAllowedUsage));
      }
    }

    last_ts_ = now;

    return result;
  }

 private:
  struct ThreadMeta final {
    clockid_t clock_id;
    std::chrono::microseconds last_usage;
  };

  static std::chrono::microseconds GetThreadCpuUsage(clockid_t cid) {
    timespec ts{};
    if (clock_gettime(cid, &ts)) {
      LOG_LIMITED_ERROR()
          << "clock_gettime call failed unexpectedly, something went terribly "
             "wrong. CPU load for a given thread will be a dummy (zero)";
      return std::chrono::microseconds{0};
    }

    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::nanoseconds{
            (static_cast<std::uint64_t>(ts.tv_sec) * 1'000'000'000 +
             static_cast<std::uint64_t>(ts.tv_nsec))});
  };

  using Clock = std::chrono::steady_clock;

  Clock::time_point last_ts_{};
  std::vector<ThreadMeta> threads_;

  bool failed_at_initialization_{false};
};
#else
class ThreadPoolCpuStatsStorage::Impl final {
 public:
  Impl(const std::vector<std::thread>& threads)
      : threads_count_{threads.size()} {}

  std::vector<Percent> Collect() const {
    return std::vector<Percent>(threads_count_, 0);
  }

 private:
  std::size_t threads_count_;
};
#endif

ThreadPoolCpuStatsStorage::ThreadPoolCpuStatsStorage(
    const std::vector<std::thread>& threads)
    : impl_{threads} {}

ThreadPoolCpuStatsStorage::~ThreadPoolCpuStatsStorage() = default;

std::vector<Percent> ThreadPoolCpuStatsStorage::CollectCurrentLoadPct() {
  return impl_->Collect();
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
