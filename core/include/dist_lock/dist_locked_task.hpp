#pragma once

/// @file dist_lock/dist_locked_task.hpp
/// @brief @copybrief dist_lock::DistLockedTask

#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include <boost/optional.hpp>

#include <dist_lock/dist_lock_settings.hpp>
#include <dist_lock/dist_lock_strategy.hpp>
#include <engine/task/task_with_result.hpp>

namespace engine {

class TaskProcessor;

}  // namespace engine

namespace dist_lock {
namespace impl {

class Locker;

}  // namespace impl

/// A task that tries to acquire a distributed lock and runs user callback once
/// while the lock is held.
/// @see AlwaysBusyDistLockStrategy
class DistLockedTask final : public engine::TaskWithResult<void> {
 public:
  using WorkerFunc = std::function<void()>;

  /// Default constructor.
  /// Creates an invalid task.
  DistLockedTask() = default;

  /// Creates a DistLockedTask.
  /// @param name name of the task
  /// @param worker_func a callback that is started once we've acquired the lock
  /// and is cancelled when the lock is lost.
  /// @param settings distributed lock settings
  /// @param strategy distributed locking strategy
  /// @param mode distributed lock waiting mode
  /// @note `worker_func` must honour task cancellation and stop ASAP when
  /// it is cancelled, otherwise brain split is possible (IOW, two different
  /// users do work assuming both of them hold the lock, which is not true).
  DistLockedTask(std::string name, WorkerFunc worker_func,
                 std::shared_ptr<DistLockStrategyBase> strategy,
                 const DistLockSettings& settings = {},
                 DistLockWaitingMode mode = DistLockWaitingMode::kWait);

  /// Creates a DistLockedTask to be run in a specific engine::TaskProcessor
  DistLockedTask(engine::TaskProcessor& task_processor, std::string name,
                 WorkerFunc worker_func,
                 std::shared_ptr<DistLockStrategyBase> strategy,
                 const DistLockSettings& settings = {},
                 DistLockWaitingMode mode = DistLockWaitingMode::kWait);

  /// Returns for how long the lock is held (if held at all). Returned value
  /// may be less than the real duration.
  boost::optional<std::chrono::steady_clock::duration> GetLockedDuration()
      const;

 private:
  DistLockedTask(engine::TaskProcessor&, std::shared_ptr<impl::Locker>,
                 DistLockWaitingMode);

  std::shared_ptr<impl::Locker> locker_ptr_;
};

}  // namespace dist_lock
