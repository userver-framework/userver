#include <userver/dist_lock/dist_locked_task.hpp>

#include <userver/engine/async.hpp>
#include <userver/tracing/span.hpp>

#include <dist_lock/impl/helpers.hpp>
#include <dist_lock/impl/locker.hpp>
#include <userver/engine/task/cancel.hpp>

USERVER_NAMESPACE_BEGIN

namespace dist_lock {

DistLockedTask::DistLockedTask(std::string name, WorkerFunc worker_func,
                               std::shared_ptr<DistLockStrategyBase> strategy,
                               const DistLockSettings& settings,
                               DistLockWaitingMode mode,
                               DistLockRetryMode retry_mode)
    : DistLockedTask(engine::current_task::GetTaskProcessor(),
                     std::make_shared<impl::Locker>(
                         std::move(name), std::move(strategy), settings,
                         std::move(worker_func), retry_mode),
                     mode) {}

DistLockedTask::DistLockedTask(engine::TaskProcessor& task_processor,
                               std::string name, WorkerFunc worker_func,
                               std::shared_ptr<DistLockStrategyBase> strategy,
                               const DistLockSettings& settings,
                               DistLockWaitingMode mode,
                               DistLockRetryMode retry_mode)
    : DistLockedTask(task_processor,
                     std::make_shared<impl::Locker>(
                         std::move(name), std::move(strategy), settings,
                         std::move(worker_func), retry_mode),
                     mode) {}

DistLockedTask::DistLockedTask(engine::TaskProcessor& task_processor,
                               std::shared_ptr<impl::Locker> locker_ptr,
                               DistLockWaitingMode mode)
    : TaskBase(locker_ptr->RunAsync(task_processor, impl::LockerMode::kOneshot,
                                    mode)),
      locker_ptr_(std::move(locker_ptr)) {}

DistLockedTask::~DistLockedTask() {
  if (IsValid()) {
    RequestCancel();

    engine::TaskCancellationBlocker cancel_blocker;
    Wait();
  }
}

std::optional<std::chrono::steady_clock::duration>
DistLockedTask::GetLockedDuration() const {
  return locker_ptr_->GetLockedDuration();
}

void DistLockedTask::Get() noexcept(false) {
  UINVARIANT(IsValid(),
             "DistLockedTask::Get was called on an invalid task. Note that "
             "Get invalidates self, so it must be called at most once "
             "per task");

  Wait();
  if (GetState() == State::kCancelled) {
    throw engine::TaskCancelledException(CancellationReason());
  }

  utils::FastScopeGuard invalidate([this]() noexcept { Invalidate(); });
  utils::impl::CastWrappedCall<void>(GetPayload()).Retrieve();
}

}  // namespace dist_lock

USERVER_NAMESPACE_END
