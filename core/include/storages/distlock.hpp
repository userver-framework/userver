#pragma once

#include <atomic>
#include <chrono>

#include <engine/mutex.hpp>
#include <engine/task/task_with_result.hpp>

namespace storages {

struct LockedTaskSettings {
  /// How often to try to acquire the lock.
  std::chrono::milliseconds acquire_interval{100};

  /// How often to try to prolong the lock while holding the lock.
  std::chrono::milliseconds prolong_attempt_interval{100};

  /// For how long to acquire/prolong the lock.
  std::chrono::milliseconds prolong_interval{1000};

  /// How much time with the held lock we treat as "we're loosing the lock, may
  /// not expect to be the only masterfrom now on".
  std::chrono::milliseconds prolong_critical_interval{50};

  std::chrono::steady_clock::time_point GetDeadline(
      std::chrono::steady_clock::time_point lock_time_point) const;

  std::chrono::milliseconds GetEffectiveWatchdogInterval() const;
};

/// A high-level primitive that tries to acquire an abstract lock and runs user
/// callback in a separate task while the lock is held. Cancels the task when
/// the lock is lost.
class LockedTask {
 public:
  using WorkerFunc = std::function<void()>;

  /// Creates a LockedTask. worker_func is a callback that is started each time
  /// we've acquired the lock and is cancelled each time the lock is lost.
  /// worker_func must honour task cancellation and stop ASAP if it is
  /// cancelled, otherwise brain split is possible (IOW, two different users do
  /// work assuming both of them hold the lock, which is not true).
  explicit LockedTask(std::string name, WorkerFunc worker_func,
                      const LockedTaskSettings& settings);

  virtual ~LockedTask();

  /// Update settings in a thread-safe way.
  void UpdateSettings(const LockedTaskSettings& settings);

  /// Starts acquiring the lock. Please note that it's possible that the lock is
  /// acquired and the WorkerFunc is entered *before* Start() returns.
  /// @see LockedTask::LockedTask
  void Start();

  /// Stops acquiring the lock. It is guaranteed that the lock is not held after
  /// Stop() return and WorkerFunc is stopped (if was started).
  void Stop();

  /// @returns whether the LockedTask is started.
  bool IsRunning() const;

 protected:
  class LockIsAcquiredByAnotherHostError : public std::exception {};

  /// Get lock or prolong for the specified timeframe. Returns without exception
  /// on success.
  /// @throws LockIsAcquiredByAnotherHostError if the lock may not be acquired
  /// and worker must be immediatelly stopped to prevent split brain.
  /// @throws std::exception on temporary failure to prolong the lock (e.g.
  /// socket error).
  virtual void RequestAcquire(std::chrono::milliseconds lock_time) = 0;

  /// Gracefully release the lock.
  /// @throws std::exception
  virtual void RequestRelease() = 0;

  /// Somehow handle "brain split identified" case, e.g. call _exit().
  /// Defaults to "do nothing".
  virtual void OnBrainSplit() {}

 private:
  void DoLocker();

  void DoLockerCycle();

  void DoWatchdog();

  LockedTaskSettings GetSettings();

  void ChangeLockStatus(bool locked);

  void BrainSplit();

 private:
  const std::string name_;
  WorkerFunc worker_func_;

  mutable engine::Mutex worker_mutex_;
  engine::TaskWithResult<void> worker_task_;

  mutable engine::Mutex mutex_;
  engine::TaskWithResult<void> locker_task_;
  engine::TaskWithResult<void> watchdog_task_;

  engine::Mutex settings_mutex_;
  LockedTaskSettings settings_;

  std::atomic<bool> locked_;
  std::atomic<std::chrono::steady_clock::duration> locked_time_point_;
};

}  // namespace storages
