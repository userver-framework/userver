#pragma once

/// @file utils/background_task_storage.hpp
/// @brief @copybrief utils::BackgroundTasksStorage

#include <utils/async.hpp>
#include <utils/impl/detached_tasks_sync_block.hpp>
#include <utils/impl/wait_token_storage.hpp>

namespace utils {

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
  void AsyncDetach(const std::string& name, Function f, Args&&... args) {
    auto task = std::make_shared<engine::TaskWithResult<void>>();
    auto handle = sync_block_.Add(task);
    TaskRemoveGuard remove_guard(handle, sync_block_);

    *task = utils::Async(
        name,
        [f = std::move(f), token = wts_.GetToken(),
         remove_guard = std::move(remove_guard)](auto&&... args) {
          f(std::forward<decltype(args)>(args)...);
        },
        std::forward<Args>(args)...);

    utils::Async(name, [task = std::move(task)]() { task->Wait(); }).Detach();
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
