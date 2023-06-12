#include <userver/engine/impl/task_context_holder.hpp>

#include <utility>

#include <engine/task/task_context.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

TaskContextHolder::TaskContextHolder(
    boost::intrusive_ptr<TaskContext>&& context) noexcept
    : context_(std::move(context)) {}

TaskContextHolder TaskContextHolder::Adopt(TaskContext& context) noexcept {
  return TaskContextHolder(
      boost::intrusive_ptr<TaskContext>{&context, /*add_ref=*/false});
}

TaskContextHolder::~TaskContextHolder() = default;

boost::intrusive_ptr<TaskContext>&& TaskContextHolder::Extract() && noexcept {
  UASSERT(context_);
  return std::move(context_);
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
