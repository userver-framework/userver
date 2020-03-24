#pragma once

#include <atomic>
#include <functional>
#include <string>

#include <boost/optional.hpp>

#include <dist_lock/dist_lock_settings.hpp>
#include <dist_lock/dist_lock_strategy.hpp>
#include <dist_lock/statistics.hpp>
#include <engine/mutex.hpp>

namespace dist_lock::impl {

enum class LockerMode {
  kOneshot,  ///< runs the payload once, then returns
  kWorker,   ///< reacquires lock and runs the payload indefinitely
};

class Locker final {
 public:
  Locker(std::string name, std::shared_ptr<DistLockStrategyBase> strategy,
         const DistLockSettings& settings, std::function<void()> worker_func);

  const std::string& Name() const;

  DistLockSettings GetSettings() const;
  void SetSettings(const DistLockSettings&);

  boost::optional<std::chrono::steady_clock::duration> GetLockedDuration()
      const;

  const Statistics& GetStatistics() const;

  void Run(LockerMode, dist_lock::DistLockWaitingMode);

 private:
  class LockGuard;

  /// @returns previous state
  bool ExchangeLockState(bool is_locked,
                         std::chrono::steady_clock::time_point when);

  void RunWatchdog();

  const std::string name_;
  std::shared_ptr<DistLockStrategyBase> strategy_;
  std::function<void()> worker_func_;

  mutable engine::Mutex settings_mutex_;
  DistLockSettings settings_;

  std::atomic<bool> is_locked_{false};
  std::atomic<std::chrono::steady_clock::duration> lock_refresh_since_epoch_{};
  std::atomic<std::chrono::steady_clock::duration> lock_acquire_since_epoch_{};

  Statistics stats_;
};

}  // namespace dist_lock::impl
