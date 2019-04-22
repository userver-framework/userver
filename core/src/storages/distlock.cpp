#include <storages/distlock.hpp>

#include <engine/sleep.hpp>
#include <engine/task/cancel.hpp>
#include <engine/task/task_context.hpp>
#include <utils/async.hpp>

namespace storages {

namespace {
void GetTask(engine::TaskWithResult<void>& task, const std::string& name) {
  try {
    if (task.IsValid()) task.Get();
  } catch (const engine::TaskCancelledException& e) {
    // Do nothing
  } catch (const std::exception& e) {
    LOG_ERROR() << "Unexpected exception on " << name << ".Get(): " << e;
  }
}
}  // namespace

std::chrono::steady_clock::time_point LockedTaskSettings::GetDeadline(
    std::chrono::steady_clock::time_point lock_time_point) const {
  return lock_time_point + prolong_interval - prolong_critical_interval;
}

std::chrono::milliseconds LockedTaskSettings::GetEffectiveWatchdogInterval()
    const {
  return prolong_critical_interval / 2;
}

LockedTask::LockedTask(std::string name, WorkerFunc worker_func,
                       const LockedTaskSettings& settings)
    : name_(std::move(name)),
      worker_func_(std::move(worker_func)),
      settings_(settings),
      locked_(false),
      locked_time_point_{} {}

LockedTask::~LockedTask() { UASSERT_MSG(!IsRunning(), "Stop() is not called"); }

void LockedTask::UpdateSettings(const LockedTaskSettings& settings) {
  std::unique_lock<engine::Mutex> lock(settings_mutex_);
  settings_ = settings;
}

void LockedTask::Start() {
  LOG_INFO() << "Starting LockedTask " << name_;

  std::unique_lock<engine::Mutex> lock(mutex_);
  locker_task_ =
      utils::CriticalAsync("locker-" + name_, [this] { DoLocker(); });
  watchdog_task_ =
      utils::CriticalAsync("watchdog-" + name_, [this] { DoWatchdog(); });

  LOG_DEBUG() << "Started LockedTask " << name_;
}

void LockedTask::Stop() {
  LOG_INFO() << "Stopping LockedTask " << name_;

  std::unique_lock<engine::Mutex> lock(mutex_);

  if (locker_task_.IsValid()) locker_task_.RequestCancel();
  if (watchdog_task_.IsValid()) watchdog_task_.RequestCancel();

  GetTask(locker_task_, "locker_task_");
  GetTask(watchdog_task_, "watchdog_task_");

  LOG_INFO() << "Stopped LockedTask " << name_;
}

bool LockedTask::IsRunning() const {
  std::unique_lock<engine::Mutex> lock(mutex_);
  return locker_task_.IsValid();
}

void LockedTask::DoLocker() {
  ChangeLockStatus(false);

  try {
    DoLockerCycle();
    ChangeLockStatus(false);
  } catch (...) {
    ChangeLockStatus(false);
    throw;
  }
}

void LockedTask::DoLockerCycle() {
  while (!engine::current_task::IsCancelRequested()) {
    auto settings = GetSettings();

    // Relax
    auto period = locked_.load() ? settings.prolong_attempt_interval
                                 : settings.acquire_interval;
    engine::InterruptibleSleepFor(period);
    if (engine::current_task::IsCancelRequested()) break;

    // Prolong
    try {
      auto attemp_start_time_point =
          utils::datetime::SteadyNow().time_since_epoch();
      RequestAcquire(settings.prolong_interval);

      // Set locked_time_point_ before locked_ as the watchdog must see
      // up-to-date time point value
      locked_time_point_.store(attemp_start_time_point,
                               std::memory_order_relaxed);
      ChangeLockStatus(true);
    } catch (const LockIsAcquiredByAnotherHostError& e) {
      if (locked_) BrainSplit();
    } catch (const std::exception& e) {
      LOG_WARNING() << "Prolong/Acquire failed with exception: " << e;
    }
  }
}

void LockedTask::DoWatchdog() {
  while (!engine::current_task::IsCancelRequested()) {
    auto settings = GetSettings();
    engine::InterruptibleSleepFor(settings.GetEffectiveWatchdogInterval());

    auto now = utils::datetime::SteadyNow();

    if (locked_.load()) {
      auto deadline = settings.GetDeadline(
          std::chrono::steady_clock::time_point(locked_time_point_.load()));
      if (deadline < now) {
        LOG_ERROR() << "Failed to prolong the lock before the deadline, "
                       "voluntarily dropping the lock and killing the worker "
                       "task to avoid brain split";
        ChangeLockStatus(false);
      } else {
        LOG_DEBUG() << "Watchdog found a valid locked timepoint ("
                    << deadline.time_since_epoch().count() << " < "
                    << now.time_since_epoch().count() << ")";
      }
    }
  }
}

LockedTaskSettings LockedTask::GetSettings() {
  std::unique_lock<engine::Mutex> lock(settings_mutex_);
  return settings_;
}

void LockedTask::ChangeLockStatus(bool locked) {
  if (locked_ == locked) {
    LOG_DEBUG() << "Lock status is the same, doing nothing (1)";
    return;
  }

  std::unique_lock<engine::Mutex> lock(worker_mutex_);
  if (locked_ == locked) {
    LOG_DEBUG() << "Lock status is the same, doing nothing (2)";
    return;
  }
  locked_ = locked;

  if (!locked) {
    if (worker_task_.IsValid()) {
      LOG_INFO() << "Cancelling worker task";
      worker_task_.RequestCancel();

      // Synchronously wait for worker exit
      // We don't want to re-lock until current worker is dead for sure
      GetTask(worker_task_, "worker_task_");
      LOG_INFO() << "Worker task is cancelled";
    } else {
      LOG_INFO() << "Lock is lost, but worker is not running";
    }
  } else {
    LOG_INFO() << "Acquired the lock, starting worker task";
    worker_task_ = utils::CriticalAsync("lock-worker-" + name_, worker_func_);
    LOG_INFO() << "Worker task is started";
  }
}

void LockedTask::BrainSplit() {
  LOG_ERROR() << "LockedTask brain split detected! Someone else acquired the "
                 "lock while we're assuming we're holding the lock. It may be "
                 "a brain split in DB backend or missing cancellation point in "
                 "worker code.";
  ChangeLockStatus(false);
  OnBrainSplit();
}

}  // namespace storages
