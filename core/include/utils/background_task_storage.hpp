#pragma once

/// @file utils/background_task_storage.hpp
/// @brief @copybrief utils::BackgroundTasksStorage

#include <utils/async.hpp>
#include <utils/impl/detached_tasks_sync_block.hpp>
#include <utils/impl/wait_token_storage.hpp>

namespace utils {

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
///   BackgroundTasksStorage bts;
///
///   bts.AsyncDetach("task", [&x]{
///     engine::InterruptibleSleepFor(std::chrono::seconds(60));
///     // this sleep will be cancelled
///     ...
///   });
///   // bts.~BackgroundTasksStorage() requests tasks cancellation
///   // and waits for detached tasks to finish
/// }
/// ```
class BackgroundTasksStorage final {
 public:
  ~BackgroundTasksStorage() {
    sync_block_.RequestCancellation();
    wts_.WaitForAllTokens();
  }

  BackgroundTasksStorage() = default;
  BackgroundTasksStorage(const BackgroundTasksStorage&) = delete;
  BackgroundTasksStorage& operator=(const BackgroundTasksStorage&) = delete;

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
  long ActiveTasksApprox() { return wts_.AliveTokensApprox(); }

 private:
  class TaskRemoveGuard final {
   public:
    TaskRemoveGuard(impl::DetachedTasksSyncBlock::TasksStorage::iterator iter,
                    impl::DetachedTasksSyncBlock& sync_block)
        : iter_(iter), sync_block_(sync_block) {}

    ~TaskRemoveGuard() {
      if (!invalidated) sync_block_.Remove(iter_);
    }

    TaskRemoveGuard(TaskRemoveGuard&& other) noexcept
        : iter_(other.iter_),
          sync_block_(other.sync_block_),
          invalidated(std::exchange(other.invalidated, true)) {}

   private:
    impl::DetachedTasksSyncBlock::TasksStorage::iterator iter_;
    impl::DetachedTasksSyncBlock& sync_block_;
    bool invalidated{false};
  };

  impl::DetachedTasksSyncBlock sync_block_;
  impl::WaitTokenStorage wts_;
};

}  // namespace utils
