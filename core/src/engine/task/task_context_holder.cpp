#include <userver/engine/task/task_context_holder.hpp>

#include "task_context.hpp"

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

TaskContextHolder::TaskContextHolder(
    boost::intrusive_ptr<TaskContext>&& context)
    : context_(std::move(context)) {}

TaskContextHolder::~TaskContextHolder() = default;
TaskContextHolder::TaskContextHolder(TaskContextHolder&&) noexcept = default;
TaskContextHolder& TaskContextHolder::operator=(TaskContextHolder&&) noexcept =
    default;

TaskContextHolder TaskContextHolder::MakeContext(TaskProcessor& task_processor,
                                                 Task::Importance importance,
                                                 Deadline deadline,
                                                 Payload&& payload) {
  return TaskContextHolder(new TaskContext(task_processor, importance, deadline,
                                           std::move(payload)));
}

boost::intrusive_ptr<TaskContext> TaskContextHolder::Release() {
  return std::move(context_);
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
