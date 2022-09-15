#pragma once

/// @file userver/concurrent/background_task_storage.hpp
/// @brief @copybrief concurrent::BackgroundTaskStorage

#include <cstdint>
#include <functional>
#include <utility>

#include <userver/engine/impl/detached_tasks_sync_block.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

/// @ingroup userver_concurrency userver_containers
///
/// A storage that allows one to start detached tasks and wait for their
/// completion at the storage destructor. All active tasks are cancelled at an
/// explicit CancelAndWait call (recommended) or at the storage destructor.
///
/// Usable for detached tasks that capture references to resources with a
/// limited lifetime. You must guarantee that the resources are available while
/// the BackgroundTaskStorage is alive.
///
/// ## Usage synopsis
/// ```
/// {
///   int x;
///   // You must guarantee that 'x' is alive during 'bts' lifetime.
///   BackgroundTaskStorage bts;
///
///   bts.AsyncDetach("task", [&x]{
///     engine::InterruptibleSleepFor(std::chrono::seconds(60));
///     // this sleep will be cancelled
///     ...
///   });
///   // bts.~BackgroundTaskStorage() requests tasks cancellation
///   // and waits for detached tasks to finish
/// }
/// ```
class BackgroundTaskStorage final {
 public:
  /// Construct an empty BTS (with no launched tasks)
  BackgroundTaskStorage();

  BackgroundTaskStorage(const BackgroundTaskStorage&) = delete;
  BackgroundTaskStorage& operator=(const BackgroundTaskStorage&) = delete;
  ~BackgroundTaskStorage();

  /// Explicitly cancel and wait for the tasks. New tasks must not be launched
  /// after this call returns. Should be called no more than once.
  void CancelAndWait() noexcept;

  /// Launch a task that will be cancelled and waited for in the BTS destructor.
  /// See utils::Async for parameter details.
  ///
  /// @see server::request::kTaskInheritedData Stops deadline propagation.
  template <typename... Args>
  void AsyncDetach(std::string name, Args&&... args) {
    Detach(utils::Async(std::move(name), HandlerInheritedDataDropper{},
                        std::forward<Args>(args)...));
  }

  /// @overload
  template <typename... Args>
  void AsyncDetach(engine::TaskProcessor& task_processor, std::string name,
                   Args&&... args) {
    Detach(utils::Async(task_processor, std::move(name),
                        HandlerInheritedDataDropper{},
                        std::forward<Args>(args)...));
  }

  /// @brief Detaches task, allowing it to continue execution out of scope. It
  /// will be cancelled and waited for on BTS destruction.
  /// @note After detach, Task becomes invalid
  void Detach(engine::Task&& task);

  /// Approximate number of currently active tasks
  std::int64_t ActiveTasksApprox() const noexcept;

 private:
  static void DropHandlerInheritedData();

  struct HandlerInheritedDataDropper final {
    template <typename... Args>
    auto operator()(Args&&... args) {
      DropHandlerInheritedData();
      return std::invoke(std::forward<Args>(args)...);
    }
  };

  std::optional<engine::impl::DetachedTasksSyncBlock> sync_block_;
};

}  // namespace concurrent

USERVER_NAMESPACE_END
