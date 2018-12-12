#include <engine/task/task_context_holder.hpp>

#include "task_context.hpp"

namespace engine {
namespace impl {

TaskContextHolder::TaskContextHolder(
    boost::intrusive_ptr<TaskContext>&& context)
    : context_(std::move(context)) {}

TaskContextHolder::~TaskContextHolder() = default;
TaskContextHolder::TaskContextHolder(TaskContextHolder&&) noexcept = default;
TaskContextHolder& TaskContextHolder::operator=(TaskContextHolder&&) noexcept =
    default;

TaskContextHolder TaskContextHolder::MakeContext(TaskProcessor& task_processor,
                                                 Task::Importance importance,
                                                 Payload payload) {
  return TaskContextHolder(
      new TaskContext(task_processor, importance, std::move(payload)));
}

boost::intrusive_ptr<TaskContext> TaskContextHolder::Release() {
  return std::move(context_);
}

}  // namespace impl
}  // namespace engine
