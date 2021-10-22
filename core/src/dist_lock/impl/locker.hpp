#pragma once

#include <atomic>
#include <functional>
#include <optional>
#include <string>

#include <userver/dist_lock/dist_lock_settings.hpp>
#include <userver/dist_lock/dist_lock_strategy.hpp>
#include <userver/dist_lock/statistics.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace dist_lock::impl {

enum class LockerMode {
  kOneshot,  ///< runs the payload once, then returns
  kWorker,   ///< reacquires lock and runs the payload indefinitely
};

class Locker final {
 public:
  Locker(std::string name, std::shared_ptr<DistLockStrategyBase> strategy,
         const DistLockSettings& settings, std::function<void()> worker_func,
         DistLockRetryMode retry_mode = DistLockRetryMode::kRetry);

  const std::string& Name() const;
  const std::string& Id() const;

  DistLockSettings GetSettings() const;
  void SetSettings(const DistLockSettings&);

  std::optional<std::chrono::steady_clock::duration> GetLockedDuration() const;

  const Statistics& GetStatistics() const;

  engine::TaskWithResult<void> RunAsync(engine::TaskProcessor& task_processor,
                                        LockerMode, DistLockWaitingMode);

 private:
  class LockGuard;

  void Run(LockerMode, dist_lock::DistLockWaitingMode, tracing::Span&& span);

  /// @returns previous state
  bool ExchangeLockState(bool is_locked,
                         std::chrono::steady_clock::time_point when);

  void RunWatchdog();

  const std::string name_;
  const std::string id_;
  std::shared_ptr<DistLockStrategyBase> strategy_;
  std::function<void()> worker_func_;

  mutable engine::Mutex settings_mutex_;
  DistLockSettings settings_;
  const DistLockRetryMode retry_mode_;

  std::atomic<bool> is_locked_{false};
  std::atomic<std::chrono::steady_clock::duration> lock_refresh_since_epoch_{};
  std::atomic<std::chrono::steady_clock::duration> lock_acquire_since_epoch_{};

  Statistics stats_;
};

}  // namespace dist_lock::impl

USERVER_NAMESPACE_END
