#include <userver/concurrent/impl/detached_tasks_sync_block.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

DetachedTasksSyncBlock::TasksStorage::iterator DetachedTasksSyncBlock::Add(
    std::shared_ptr<engine::Task> task) {
  auto tasks = shared_tasks_.Lock();
  return tasks->emplace(tasks->end(), std::move(task));
}

void DetachedTasksSyncBlock::Remove(
    DetachedTasksSyncBlock::TasksStorage::iterator iter) {
  // task destruction may block, do it outside
  TasksStorage::value_type task;
  {
    auto tasks = shared_tasks_.Lock();
    task = std::move(*iter);
    tasks->erase(iter);
  }
}

void DetachedTasksSyncBlock::RequestCancellation() {
  auto tasks = shared_tasks_.Lock();
  for (auto& task : *tasks)
    if (task->IsValid()) task->RequestCancel();

  for (auto& task_ptr : *tasks) task_ptr.reset();
}

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
