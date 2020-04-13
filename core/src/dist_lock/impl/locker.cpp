#include <dist_lock/impl/locker.hpp>

#include <stdexcept>

#include <engine/sleep.hpp>
#include <engine/task/cancel.hpp>
#include <engine/task/task_with_result.hpp>
#include <logging/log.hpp>
#include <tracing/span.hpp>
#include <utils/assert.hpp>
#include <utils/async.hpp>
#include <utils/datetime.hpp>

#include <dist_lock/impl/helpers.hpp>

namespace dist_lock::impl {
namespace {

class WorkerFuncFailedException : public std::exception {};

}  // namespace

class Locker::LockGuard {
 public:
  LockGuard(Locker& locker) : locker_(locker) {}
  ~LockGuard() { TryUnlock(); }

  void TryUnlock() noexcept {
    engine::TaskCancellationBlocker cancel_blocker;
    try {
      if (locker_.ExchangeLockState(false, utils::datetime::SteadyNow())) {
        locker_.strategy_->Release();
      }
    } catch (const std::exception& ex) {
      LOG_WARNING() << "Failed to release lock on stop: " << ex;
    }
  }

 private:
  Locker& locker_;
};

Locker::Locker(std::string name, std::shared_ptr<DistLockStrategyBase> strategy,
               const DistLockSettings& settings,
               std::function<void()> worker_func)
    : name_(std::move(name)),
      strategy_(std::move(strategy)),
      worker_func_(std::move(worker_func)),
      settings_(settings) {
  UASSERT(strategy_);
}

const std::string& Locker::Name() const { return name_; }

DistLockSettings Locker::GetSettings() const {
  std::lock_guard<engine::Mutex> lock(settings_mutex_);
  return settings_;
}

void Locker::SetSettings(const DistLockSettings& settings) {
  std::lock_guard<engine::Mutex> lock(settings_mutex_);
  settings_ = settings;
}

std::optional<std::chrono::steady_clock::duration> Locker::GetLockedDuration()
    const {
  if (!is_locked_) return {};
  return std::chrono::steady_clock::now().time_since_epoch() -
         lock_acquire_since_epoch_.load();
}

const Statistics& Locker::GetStatistics() const { return stats_; }

void Locker::Run(LockerMode mode, dist_lock::DistLockWaitingMode waiting_mode) {
  tracing::Span span(LockerName(name_));
  LockGuard lock_guard(*this);
  engine::TaskWithResult<void> watchdog_task;
  bool worker_succeeded = false;

  while (!engine::current_task::ShouldCancel() &&
         (mode != LockerMode::kOneshot || !worker_succeeded)) {
    const auto settings = GetSettings();
    const auto attempt_start = utils::datetime::SteadyNow();

    try {
      strategy_->Acquire(settings.lock_ttl);
      stats_.successes++;
      if (!ExchangeLockState(true, attempt_start)) {
        LOG_DEBUG() << "Starting watchdog task";
        GetTask(watchdog_task, WatchdogName(name_));
        watchdog_task = utils::CriticalAsync(WatchdogName(name_),
                                             [this] { RunWatchdog(); });
        LOG_DEBUG() << "Started watchdog task";
      }
    } catch (const LockIsAcquiredByAnotherHostException&) {
      stats_.failures++;
      if (is_locked_) {
        LOG_ERROR()
            << "DistLockedTask brain split detected! Someone else acquired the "
               "lock while we're assuming we're holding the lock. It may be "
               "a brain split in DB backend or missing cancellation point in "
               "worker code.";
        stats_.brain_splits++;
        LOG_DEBUG() << "Terminating watchdog task";
        if (watchdog_task.IsValid()) watchdog_task.RequestCancel();
        GetTask(watchdog_task, WatchdogName(name_));
        LOG_DEBUG() << "Terminated watchdog task";
        ExchangeLockState(false, utils::datetime::SteadyNow());
      }
      if (waiting_mode == dist_lock::DistLockWaitingMode::kNoWait) break;
    } catch (const std::exception& ex) {
      stats_.failures++;
      LOG_WARNING() << "Lock acquisition failed: " << ex;
    }

    if (engine::current_task::ShouldCancel()) break;

    auto delay =
        is_locked_ ? settings.prolong_interval : settings.acquire_interval;
    if (watchdog_task.IsValid()) {
      try {
        watchdog_task.WaitFor(delay);
      } catch (const engine::WaitInterruptedException&) {
        // do nothing
      }

      if (watchdog_task.IsFinished()) {
        worker_succeeded = GetTask(watchdog_task, WatchdogName(name_));
        if (!worker_succeeded || mode == LockerMode::kWorker) {
          lock_guard.TryUnlock();
          engine::InterruptibleSleepFor(settings.worker_func_restart_delay);
        }
      }
    } else {
      engine::InterruptibleSleepFor(delay);
    }
  }
  if (watchdog_task.IsValid()) watchdog_task.RequestCancel();
  GetTask(watchdog_task, WatchdogName(name_));
}

bool Locker::ExchangeLockState(bool is_locked,
                               std::chrono::steady_clock::time_point when) {
  lock_refresh_since_epoch_.store(when.time_since_epoch(),
                                  std::memory_order_release);

  auto was_locked = is_locked_.exchange(is_locked);
  if (is_locked != was_locked) {
    if (!was_locked) {
      LOG_INFO() << "Acquired the lock";
      lock_acquire_since_epoch_ = when.time_since_epoch();
    } else {
      LOG_INFO() << "Released (or lost) the lock";
    }
  }
  return was_locked;
}

void Locker::RunWatchdog() {
  LOG_INFO() << "Starting worker task";
  auto worker_task =
      utils::CriticalAsync("lock-worker-" + name_, [this] { worker_func_(); });
  LOG_INFO() << "Started worker task";

  while (!engine::current_task::ShouldCancel() && !worker_task.IsFinished()) {
    const auto settings = GetSettings();

    const auto refresh_since_epoch =
        lock_refresh_since_epoch_.load(std::memory_order_acquire);
    const auto deadline =
        std::chrono::steady_clock::time_point{refresh_since_epoch} +
        settings.lock_ttl - settings.forced_stop_margin;
    const auto now = utils::datetime::SteadyNow();
    if (deadline < now) {
      LOG_ERROR() << "Failed to prolong the lock before the deadline, "
                     "voluntarily dropping the lock and killing the worker "
                     "task to avoid brain split";
      stats_.watchdog_triggers++;
      break;
    } else {
      LOG_DEBUG() << "Watchdog found a valid locked timepoint ("
                  << now.time_since_epoch().count() << " < "
                  << deadline.time_since_epoch().count() << ")";
    }

    try {
      worker_task.WaitFor(settings.forced_stop_margin / 2);
    } catch (const engine::WaitInterruptedException&) {
      // do nothing
    }
  }
  LOG_INFO() << "Waiting for worker task";
  worker_task.RequestCancel();
  if (!GetTask(worker_task, WorkerName(name_))) {
    LOG_INFO() << "Worker task failed";
    throw WorkerFuncFailedException{};
  }
  LOG_INFO() << "Worker task completed";
}

}  // namespace dist_lock::impl
