#pragma once

#include <userver/concurrent/variable.hpp>
#include <userver/engine/task/task.hpp>

#include <list>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

class DetachedTasksSyncBlock final {
 public:
  using TasksStorage = std::list<std::shared_ptr<engine::Task>>;

  DetachedTasksSyncBlock() = default;

  DetachedTasksSyncBlock(const DetachedTasksSyncBlock&) = delete;
  DetachedTasksSyncBlock(DetachedTasksSyncBlock&&) = delete;

  TasksStorage::iterator Add(std::shared_ptr<engine::Task>);
  void Remove(TasksStorage::iterator);
  void RequestCancellation();

 private:
  concurrent::Variable<TasksStorage> shared_tasks_{};
};

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
