#include <userver/concurrent/background_task_storage.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

BackgroundTaskStorage::~BackgroundTaskStorage() { DoCancelAndWait(); }

void BackgroundTaskStorage::CancelAndWait() noexcept {
  UASSERT_MSG(is_alive_, "CancelAndWait should be called no more than once");
  DoCancelAndWait();
}

void BackgroundTaskStorage::DoCancelAndWait() noexcept {
  if (!is_alive_) return;
  is_alive_ = false;
  sync_block_.RequestCancellation();
  wts_.WaitForAllTokens();
}

std::int64_t BackgroundTaskStorage::ActiveTasksApprox() {
  return wts_.AliveTokensApprox();
}

BackgroundTaskStorage::TaskRemoveGuard::TaskRemoveGuard(
    impl::DetachedTasksSyncBlock::TasksStorage::iterator iter,
    impl::DetachedTasksSyncBlock& sync_block)
    : iter_(iter), sync_block_(sync_block) {}

BackgroundTaskStorage::TaskRemoveGuard::~TaskRemoveGuard() {
  if (!invalidated) sync_block_.Remove(iter_);
}

BackgroundTaskStorage::TaskRemoveGuard::TaskRemoveGuard(
    BackgroundTaskStorage::TaskRemoveGuard&& other) noexcept
    : iter_(other.iter_),
      sync_block_(other.sync_block_),
      invalidated(std::exchange(other.invalidated, true)) {}

}  // namespace concurrent

USERVER_NAMESPACE_END
