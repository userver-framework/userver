#include <storages/dist_locked_task.hpp>

#include <engine/sleep.hpp>
#include <engine/task/cancel.hpp>
#include <engine/task/task_context.hpp>
#include <utils/async.hpp>

namespace storages {

namespace {
void GetTask(engine::TaskWithResult<void>& task, const std::string& name) {
  try {
    engine::TaskCancellationBlocker cancel_blocker;
    if (task.IsValid()) task.Get();
  } catch (const engine::TaskCancelledException& e) {
    // Do nothing
  } catch (const std::exception& e) {
    LOG_ERROR() << "Unexpected exception on " << name << ".Get(): " << e;
  }
}
}  // namespace

std::chrono::steady_clock::time_point DistLockedTaskSettings::GetDeadline(
    std::chrono::steady_clock::time_point lock_time_point) const {
  return lock_time_point + prolong_interval - prolong_critical_interval;
}

std::chrono::milliseconds DistLockedTaskSettings::GetEffectiveWatchdogInterval()
    const {
  return prolong_critical_interval / 2;
}

DistLockedTask::DistLockedTask(std::string name, WorkerFunc worker_func,
                               const DistLockedTaskSettings& settings)
    : name_(std::move(name)),
      worker_func_(std::move(worker_func)),
      settings_(settings),
      locked_(false),
      locked_time_point_{},
      locked_status_change_time_point_{} {}

DistLockedTask::~DistLockedTask() {
  UASSERT_MSG(!IsRunning(), "Stop() is not called");

  UASSERT(!worker_task_.IsValid());
  UASSERT(!locker_task_.IsValid());
  UASSERT(!watchdog_task_.IsValid());
}

void DistLockedTask::UpdateSettings(const DistLockedTaskSettings& settings) {
  std::unique_lock<engine::Mutex> lock(settings_mutex_);
  settings_ = settings;
}

void DistLockedTask::Start() {
  LOG_INFO() << "Starting DistLockedTask " << name_;

  std::unique_lock<engine::Mutex> lock(mutex_);
  locker_task_ =
      utils::CriticalAsync("locker-" + name_, [this] { DoLocker(); });
  watchdog_task_ =
      utils::CriticalAsync("watchdog-" + name_, [this] { DoWatchdog(); });

  LOG_DEBUG() << "Started DistLockedTask " << name_;
}

void DistLockedTask::Stop() {
  LOG_INFO() << "Stopping DistLockedTask " << name_;

  std::unique_lock<engine::Mutex> lock(mutex_);

  if (locker_task_.IsValid()) locker_task_.RequestCancel();
  if (watchdog_task_.IsValid()) watchdog_task_.RequestCancel();

  GetTask(locker_task_, "locker_task_");
  GetTask(watchdog_task_, "watchdog_task_");

  LOG_INFO() << "Stopped DistLockedTask " << name_;
}

bool DistLockedTask::IsRunning() const {
  std::unique_lock<engine::Mutex> lock(mutex_);
  return locker_task_.IsValid();
}

boost::optional<std::chrono::steady_clock::duration>
DistLockedTask::GetLockedDuration() const {
  auto locked_tp = locked_status_change_time_point_.load();
  auto locked = locked_.load();
  if (locked)
    return std::chrono::steady_clock::now().time_since_epoch() - locked_tp;
  else
    return boost::none;
}

const DistLockedTask::Statistics& DistLockedTask::GetStatistics() const {
  return stats_;
}

void DistLockedTask::DoLocker() {
  ChangeLockStatus(false);

  try {
    DoLockerCycle();

    if (locked_) RequestRelease();
    ChangeLockStatus(false);
  } catch (...) {
    if (locked_) {
      try {
        RequestRelease();
      } catch (const std::exception& e) {
        LOG_WARNING() << "Failed to release lock on Stop(): " << e;
      } catch (...) {
        ChangeLockStatus(false);
        throw;
      }
    }
    ChangeLockStatus(false);
    throw;
  }
}

void DistLockedTask::DoLockerCycle() {
  while (!engine::current_task::IsCancelRequested()) {
    auto settings = GetSettings();

    auto attemp_start_time_point = utils::datetime::SteadyNow();
    // Prolong
    try {
      RequestAcquire(settings.prolong_interval);
      stats_.successes++;

      // Set locked_time_point_ before locked_ as the watchdog must see
      // up-to-date time point value
      locked_time_point_.store(attemp_start_time_point.time_since_epoch(),
                               std::memory_order_relaxed);
      ChangeLockStatus(true);
    } catch (const LockIsAcquiredByAnotherHostError& e) {
      stats_.failures++;
      if (locked_) BrainSplit();
    } catch (const std::exception& e) {
      stats_.failures++;
      LOG_WARNING() << "Prolong/Acquire failed with exception: " << e;
    }

    if (engine::current_task::IsCancelRequested()) break;

    // Relax
    auto period = locked_.load() ? settings.prolong_attempt_interval
                                 : settings.acquire_interval;
    // engine::InterruptibleSleepUntil(attemp_start_time_point + period);
    engine::InterruptibleSleepFor(period);
  }
}

void DistLockedTask::DoWatchdog() {
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
        stats_.watchdog_triggers++;
        ChangeLockStatus(false);
      } else {
        LOG_DEBUG() << "Watchdog found a valid locked timepoint ("
                    << deadline.time_since_epoch().count() << " < "
                    << now.time_since_epoch().count() << ")";
      }
    }
  }
}

DistLockedTaskSettings DistLockedTask::GetSettings() {
  std::unique_lock<engine::Mutex> lock(settings_mutex_);
  return settings_;
}

void DistLockedTask::ChangeLockStatus(bool locked) {
  if (locked_ == locked) {
    LOG_DEBUG() << "Lock status is the same, doing nothing (1)";
    return;
  }

  std::unique_lock<engine::Mutex> lock(worker_mutex_);
  if (locked_ == locked) {
    LOG_DEBUG() << "Lock status is the same, doing nothing (2)";
    return;
  }

  locked_status_change_time_point_ =
      std::chrono::steady_clock::now().time_since_epoch();
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

void DistLockedTask::BrainSplit() {
  LOG_ERROR()
      << "DistLockedTask brain split detected! Someone else acquired the "
         "lock while we're assuming we're holding the lock. It may be "
         "a brain split in DB backend or missing cancellation point in "
         "worker code.";
  stats_.brain_splits++;
  ChangeLockStatus(false);
  OnBrainSplit();
}

}  // namespace storages
