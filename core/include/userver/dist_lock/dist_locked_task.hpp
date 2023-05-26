#pragma once

/// @file userver/dist_lock/dist_locked_task.hpp
/// @brief @copybrief dist_lock::DistLockedTask

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>

#include <userver/dist_lock/dist_lock_settings.hpp>
#include <userver/dist_lock/dist_lock_strategy.hpp>
#include <userver/engine/task/task_base.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace dist_lock {
namespace impl {

class Locker;

}  // namespace impl

// clang-format off

/// @ingroup userver_concurrency
///
/// @brief A task that tries to acquire a distributed lock and runs user
/// callback once while the lock is held.
///
/// When dist lock starts, the lock worker tries to take a lock in the
/// loop. If succeeded, a task is launched that executes the user code.
/// In the background, dist lock tries to extend the lock. In case of loss of
/// the lock, the user task is canceled.
///
/// ## Example with retrying
/// @snippet dist_lock/dist_lock_test.cpp Sample distributed locked task Retry
/// ## Example without retrying
/// @snippet dist_lock/dist_lock_test.cpp Sample distributed locked task SingleAttempt
///
/// @see @ref md_en_userver_periodics
/// @see AlwaysBusyDistLockStrategy

// clang-format on

class DistLockedTask final : public engine::TaskBase {
 public:
  using WorkerFunc = std::function<void()>;

  /// Default constructor.
  /// Creates an invalid task.
  DistLockedTask() = default;

  DistLockedTask(DistLockedTask&&) = delete;
  DistLockedTask& operator=(DistLockedTask&&) = delete;

  DistLockedTask(const DistLockedTask&) = delete;
  DistLockedTask& operator=(const DistLockedTask&&) = delete;

  ~DistLockedTask();

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
                 DistLockWaitingMode mode = DistLockWaitingMode::kWait,
                 DistLockRetryMode retry_mode = DistLockRetryMode::kRetry);

  /// Creates a DistLockedTask to be run in a specific engine::TaskProcessor
  DistLockedTask(engine::TaskProcessor& task_processor, std::string name,
                 WorkerFunc worker_func,
                 std::shared_ptr<DistLockStrategyBase> strategy,
                 const DistLockSettings& settings = {},
                 DistLockWaitingMode mode = DistLockWaitingMode::kWait,
                 DistLockRetryMode retry_mode = DistLockRetryMode::kRetry);

  /// Returns for how long the lock is held (if held at all). Returned value
  /// may be less than the real duration.
  std::optional<std::chrono::steady_clock::duration> GetLockedDuration() const;

  void Get() noexcept(false);

 private:
  DistLockedTask(engine::TaskProcessor&, std::shared_ptr<impl::Locker>,
                 DistLockWaitingMode);

  std::shared_ptr<impl::Locker> locker_ptr_;
};

}  // namespace dist_lock

USERVER_NAMESPACE_END
