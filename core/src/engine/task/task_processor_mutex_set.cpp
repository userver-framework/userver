#include <engine/task/task_processor_mutex_set.hpp>

#include <engine/task/task_context.hpp>
#include <engine/task/task_processor.hpp>
#include <userver/engine/task/current_task.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

TaskProcessorMutexSet::TaskProcessorMutexSet(TaskProcessor& tp, std::size_t worker_count)
    : tp_(tp),
      mutexes_(worker_count)
{}

std::lock_guard<std::mutex> TaskProcessorMutexSet::ReadLockFromCoroutine() noexcept {
    UASSERT_MSG(MayReadLock(), "Trying to lock from non-coroutine or another task processor");

    auto worker_id = impl::GetCurrentWorkerId();
    return std::lock_guard{*mutexes_[worker_id]};
}

TaskProcessorMutexSet::Lock TaskProcessorMutexSet::WriteLock() noexcept {
    return utils::GenerateFixedArray(mutexes_.size(), [&](std::size_t i) { return std::lock_guard(*mutexes_[i]); });
}

bool TaskProcessorMutexSet::MayReadLock() const noexcept {
    return current_task::GetCurrentTaskContextUnchecked() && &tp_ == &current_task::GetTaskProcessor();
}

}  // namespace engine

USERVER_NAMESPACE_END
