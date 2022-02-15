#pragma once

/// @file userver/concurrent/background_task_storage.hpp
/// @brief @copybrief concurrent::BackgroundTaskStorage

#include <userver/concurrent/impl/detached_tasks_sync_block.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/impl/wait_token_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

/// @ingroup userver_concurrency userver_containers
///
/// A storage that allows one to start detached tasks and wait for their
/// finish at the storage destructor. All not finished tasks are cancelled at
/// the storage destructor.
///
/// Usable for detached tasks that capture
/// references to resources with a limited lifetime. You must guarantee that
/// the resources are available while storage is alive.
/// Uses mutex, so might not be the best option for hot paths.
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
  ~BackgroundTaskStorage();

  BackgroundTaskStorage() = default;
  BackgroundTaskStorage(const BackgroundTaskStorage&) = delete;
  BackgroundTaskStorage& operator=(const BackgroundTaskStorage&) = delete;

  /// Explicitly cancel and wait for the tasks. New tasks must not be launched
  /// after this call returns. Should be called no more than once.
  void CancelAndWait() noexcept;

  template <typename Function, typename... Args>
  void AsyncDetach(engine::TaskProcessor& task_processor,
                   const std::string& name, Function f, Args&&... args) {
    // Function call is wrapped into a task as per usual, then a couple of
    // tokens is bound to it:
    //  * DetachedTasksSyncBlock token allows task cancellation on BTS
    //  destruction.
    //  * WaitTokenStorage token allows to wait for tasks with shared ownership.
    // The bundle is then sent to another task for synchronization.
    //
    // This way tokens may be destroyed in one of two cases:
    //  * Task finishes its execution in a normal way (Wait returns), or
    //  * Task is cancelled due to overload and ran only to destroy the payload.
    // Because of the second case we cannot store tokens in the main task
    // closure to avoid deadlocks on shared_ptr removal from sync_block.
    auto task = std::make_shared<engine::Task>();
    auto handle = sync_block_.Add(task);
    TaskRemoveGuard remove_guard(handle, sync_block_);
    *task = utils::Async(task_processor, name, std::move(f),
                         std::forward<Args>(args)...);

    // waiter is critical to avoid dealing with closure destruction order
    utils::CriticalAsync(task_processor, name,
                         [task_ = std::move(task), token = wts_.GetToken(),
                          remove_guard = std::move(remove_guard)]() mutable {
                           // move task to ensure it is destroyed before tokens
                           const auto task = std::move(task_);
                           task->Wait();
                         })
        .Detach();
  }

  template <typename Function, typename... Args>
  void AsyncDetach(const std::string& name, Function f, Args&&... args) {
    return AsyncDetach(engine::current_task::GetTaskProcessor(), name, f,
                       std::forward<Args>(args)...);
  }

  /// Approximate number of currently active tasks or -1 if storage is finalized
  std::int64_t ActiveTasksApprox();

 private:
  class TaskRemoveGuard final {
   public:
    TaskRemoveGuard(impl::DetachedTasksSyncBlock::TasksStorage::iterator iter,
                    impl::DetachedTasksSyncBlock& sync_block);

    TaskRemoveGuard(TaskRemoveGuard&& other) noexcept;
    ~TaskRemoveGuard();

   private:
    impl::DetachedTasksSyncBlock::TasksStorage::iterator iter_;
    impl::DetachedTasksSyncBlock& sync_block_;
    bool invalidated{false};
  };

  void DoCancelAndWait() noexcept;

  impl::DetachedTasksSyncBlock sync_block_;
  utils::impl::WaitTokenStorage wts_;
  bool is_alive_{true};
};

}  // namespace concurrent

USERVER_NAMESPACE_END
