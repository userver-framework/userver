#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>

#include <userver/dist_lock/dist_lock_settings.hpp>
#include <userver/dist_lock/dist_lock_strategy.hpp>
#include <userver/dist_lock/statistics.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/task/task_with_result.hpp>

USERVER_NAMESPACE_BEGIN

namespace dist_lock {
namespace impl {

class Locker;

}  // namespace impl

/// A high-level primitive that perpetually tries to acquire a distributed lock
/// and runs user callback in a separate task while the lock is held.
/// Cancels the task when the lock is lost.
class DistLockedWorker final {
 public:
  using WorkerFunc = std::function<void()>;

  /// Creates a DistLockedWorker.
  /// @param name name of the worker
  /// @param worker_func a callback that's started each time we acquire the lock
  /// and is cancelled when the lock is lost.
  /// @param settings distributed lock settings
  /// @param strategy distributed locking strategy
  /// @param task_processor TaskProcessor for running `worker_func`,
  /// using current TaskProcessor if `nullptr`
  /// @note `worker_func` must honour task cancellation and stop ASAP when
  /// it is cancelled, otherwise brain split is possible (IOW, two different
  /// users do work assuming both of them hold the lock, which is not true).
  DistLockedWorker(std::string name, WorkerFunc worker_func,
                   std::shared_ptr<DistLockStrategyBase> strategy,
                   const DistLockSettings& settings = {},
                   engine::TaskProcessor* task_processor = nullptr);

  ~DistLockedWorker();

  /// Name of the worker.
  const std::string& Name() const;

  /// Retrieves settings in a thread-safe way.
  DistLockSettings GetSettings() const;

  /// Update settings in a thread-safe way.
  void UpdateSettings(const DistLockSettings&);

  /// Starts acquiring the lock. Please note that it's possible that the lock is
  /// acquired and the WorkerFunc is entered *before* Start() returns.
  /// @see DistLockedTask::DistLockedTask
  void Start();

  /// Stops acquiring the lock. It is guaranteed that the lock is not held after
  /// Stop() return and WorkerFunc is stopped (if was started).
  void Stop();

  /// Run task once acquiring the lock.
  /// @throws std::exception rethrows exception from `worker_func`.
  void RunOnce();

  /// @returns whether the DistLockedTask is started.
  bool IsRunning() const;

  /// Returns for how long the lock is held (if held at all). Returned value
  /// may be less than the real duration.
  std::optional<std::chrono::steady_clock::duration> GetLockedDuration() const;

  /// Returns lock acquisition statistics.
  const Statistics& GetStatistics() const;

 private:
  engine::TaskProcessor& GetTaskProcessor() const noexcept;

  std::shared_ptr<impl::Locker> locker_ptr_;

  mutable engine::Mutex locker_task_mutex_;
  engine::TaskWithResult<void> locker_task_;

  engine::TaskProcessor* const task_processor_;
};

void DumpMetric(utils::statistics::Writer& writer,
                const DistLockedWorker& worker);

}  // namespace dist_lock

USERVER_NAMESPACE_END
