#include <engine/impl/task_context_accessor.hpp>

#include <utility>

#include <engine/task/task_base_impl.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

boost::intrusive_ptr<TaskContext> TaskContextAccessor::GetContext(TaskBase& task) noexcept {
    return task.pimpl_->context;
}

boost::intrusive_ptr<TaskContext> TaskContextAccessor::ExtractContext(TaskBase&& task) noexcept {
    return std::move(task.pimpl_->context);
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
