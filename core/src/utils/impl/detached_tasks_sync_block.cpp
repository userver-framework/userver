#include <utils/impl/detached_tasks_sync_block.hpp>

namespace utils::impl {

namespace {
using LockGuard = std::lock_guard<engine::Mutex>;
}
DetachedTasksSyncBlock::TasksStorage::iterator DetachedTasksSyncBlock::Add(
    std::shared_ptr<engine::Task> task) {
  auto tasks = shared_tasks_.Lock();
  return tasks->emplace(tasks->end(), std::move(task));
}

void DetachedTasksSyncBlock::Remove(
    DetachedTasksSyncBlock::TasksStorage::iterator iter) {
  auto tasks = shared_tasks_.Lock();
  tasks->erase(iter);
}

void DetachedTasksSyncBlock::RequestCancellation() {
  auto tasks = shared_tasks_.Lock();
  for (auto& task : *tasks)
    if (task->IsValid()) task->RequestCancel();
}

}  // namespace utils::impl
