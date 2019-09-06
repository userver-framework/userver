#include <dist_lock/dist_locked_task.hpp>

#include <engine/task/task_context_holder.hpp>

#include <dist_lock/impl/locker.hpp>

namespace dist_lock {

DistLockedTask::DistLockedTask(std::string name, WorkerFunc worker_func,
                               std::shared_ptr<DistLockStrategyBase> strategy,
                               const DistLockSettings& settings)
    : DistLockedTask(
          engine::current_task::GetTaskProcessor(),
          std::make_shared<impl::Locker>(std::move(name), std::move(strategy),
                                         settings, std::move(worker_func))) {}

DistLockedTask::DistLockedTask(engine::TaskProcessor& task_processor,
                               std::string name, WorkerFunc worker_func,
                               std::shared_ptr<DistLockStrategyBase> strategy,
                               const DistLockSettings& settings)
    : DistLockedTask(task_processor, std::make_shared<impl::Locker>(
                                         std::move(name), std::move(strategy),
                                         settings, std::move(worker_func))) {}

DistLockedTask::DistLockedTask(engine::TaskProcessor& task_processor,
                               std::shared_ptr<impl::Locker> locker_ptr)
    : Task(engine::impl::TaskContextHolder::MakeContext(
          task_processor, engine::Task::Importance::kNormal,
          [locker_ptr] { locker_ptr->Run(impl::LockerMode::kOneshot); })),
      locker_ptr_(std::move(locker_ptr)) {}

boost::optional<std::chrono::steady_clock::duration>
DistLockedTask::GetLockedDuration() const {
  return locker_ptr_->GetLockedDuration();
}

}  // namespace dist_lock
