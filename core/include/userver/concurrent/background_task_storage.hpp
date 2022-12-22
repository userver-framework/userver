#pragma once

/// @file userver/concurrent/background_task_storage.hpp
/// @brief @copybrief concurrent::BackgroundTaskStorage

#include <cstdint>
#include <utility>

#include <userver/engine/impl/detached_tasks_sync_block.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

/// @ingroup userver_concurrency userver_containers
///
/// A version of concurrent::BackgroundTaskStorage for advanced use cases (e.g.
/// driver internals) that can take the ownership of any kind of task.
class BackgroundTaskStorageCore final {
 public:
  /// Creates an empty BTS.
  BackgroundTaskStorageCore();

  BackgroundTaskStorageCore(BackgroundTaskStorageCore&&) = delete;
  BackgroundTaskStorageCore& operator=(BackgroundTaskStorageCore&&) = delete;
  ~BackgroundTaskStorageCore();

  /// Explicitly cancel and wait for the tasks. New tasks must not be launched
  /// after this call returns. Should be called no more than once.
  void CancelAndWait() noexcept;

  /// @brief Detaches task, allowing it to continue execution out of scope. It
  /// will be cancelled and waited for on BTS destruction.
  /// @note After detach, Task becomes invalid
  void Detach(engine::Task&& task);

  /// Approximate number of currently active tasks
  std::int64_t ActiveTasksApprox() const noexcept;

 private:
  std::optional<engine::impl::DetachedTasksSyncBlock> sync_block_;
};

/// @ingroup userver_concurrency userver_containers
///
/// A storage that allows one to start detached tasks; cancels and waits for
/// unfinished tasks completion at the destructor. Provides CancelAndWait to
/// explicitly cancel tasks (recommended).
///
/// Usable for detached tasks that capture references to resources with a
/// limited lifetime. You must guarantee that the resources are available while
/// the BackgroundTaskStorage is alive.
///
/// ## Usage synopsis
/// @snippet concurrent/background_task_storage_test.cpp  Sample
class BackgroundTaskStorage final {
 public:
  /// Creates a BTS that launches tasks in the engine::TaskProcessor used at the
  /// BTS creation.
  BackgroundTaskStorage();

  /// Creates a BTS that launches tasks in the specified engine::TaskProcessor.
  explicit BackgroundTaskStorage(engine::TaskProcessor& task_processor);

  BackgroundTaskStorage(const BackgroundTaskStorage&) = delete;
  BackgroundTaskStorage& operator=(const BackgroundTaskStorage&) = delete;

  /// Explicitly cancel and wait for the tasks. New tasks must not be launched
  /// after this call returns. Should be called no more than once.
  void CancelAndWait() noexcept;

  /// @brief Launch a task that will be cancelled and waited for in the BTS
  /// destructor.
  ///
  /// The task is started as non-Critical, it may be cancelled due to
  /// `TaskProcessor` overload. engine::TaskInheritedVariable instances are not
  /// inherited from the caller. See utils::AsyncBackground for details.
  template <typename... Args>
  void AsyncDetach(std::string name, Args&&... args) {
    core_.Detach(utils::AsyncBackground(std::move(name), task_processor_,
                                        std::forward<Args>(args)...));
  }

  /// @deprecated Pass engine::TaskProcessor to BTS constructor instead.
  template <typename... Args>
  void AsyncDetach(engine::TaskProcessor& task_processor, std::string name,
                   Args&&... args) {
    core_.Detach(utils::AsyncBackground(std::move(name), task_processor,
                                        std::forward<Args>(args)...));
  }

  /// @deprecated Use AsyncDetach or BackgroundTaskStorageCore instead.
  void Detach(engine::Task&& task) { core_.Detach(std::move(task)); }

  /// Approximate number of currently active tasks
  std::int64_t ActiveTasksApprox() const noexcept;

 private:
  BackgroundTaskStorageCore core_;
  engine::TaskProcessor& task_processor_;
};

}  // namespace concurrent

USERVER_NAMESPACE_END
