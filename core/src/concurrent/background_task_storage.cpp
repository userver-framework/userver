#include <userver/concurrent/background_task_storage.hpp>

#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

BackgroundTaskStorageCore::BackgroundTaskStorageCore()
    : sync_block_(
          std::in_place,
          engine::impl::DetachedTasksSyncBlock::StopMode::kCancelAndWait) {}

BackgroundTaskStorageCore::~BackgroundTaskStorageCore() {
  if (!sync_block_) return;
  sync_block_->RequestCancellation(engine::TaskCancellationReason::kAbandoned);
}

void BackgroundTaskStorageCore::CancelAndWait() noexcept {
  UASSERT_MSG(sync_block_, "CancelAndWait should be called no more than once");
  if (!sync_block_) return;
  sync_block_->RequestCancellation(
      engine::TaskCancellationReason::kUserRequest);
  sync_block_.reset();
}

void BackgroundTaskStorageCore::Detach(engine::Task&& task) {
  UINVARIANT(sync_block_, "Trying to launch a task on a dead BTS");
  sync_block_->Add(std::move(task));
}

std::int64_t BackgroundTaskStorageCore::ActiveTasksApprox() const noexcept {
  UASSERT_MSG(sync_block_, "Trying to get the task count for a dead BTS");
  if (!sync_block_) return 0;
  return sync_block_->ActiveTasksApprox();
}

BackgroundTaskStorage::BackgroundTaskStorage()
    : BackgroundTaskStorage(engine::current_task::GetTaskProcessor()) {}

BackgroundTaskStorage::BackgroundTaskStorage(
    engine::TaskProcessor& task_processor)
    : task_processor_(task_processor) {}

void BackgroundTaskStorage::CancelAndWait() noexcept { core_.CancelAndWait(); }

std::int64_t BackgroundTaskStorage::ActiveTasksApprox() const noexcept {
  return core_.ActiveTasksApprox();
}

}  // namespace concurrent

USERVER_NAMESPACE_END
