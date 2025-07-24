#include <userver/engine/task/task.hpp>

#include <future>

#include <engine/impl/generic_wait_list.hpp>
#include <engine/task/task_context.hpp>
#include <engine/task/task_processor.hpp>
#include <engine/task/task_processor_pools.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/impl/task_context_holder.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

static_assert(!std::is_polymorphic_v<Task>, "Slicing is used by derived types, virtual functions would not work.");

Task::Task() { UASSERT(!IsValid()); }

Task::Task(impl::TaskContextHolder&& context) : TaskBase(std::move(context)) {}

Task::Task(Task&& other) noexcept : TaskBase(static_cast<TaskBase&&>(other)) { UASSERT(!other.IsValid()); }

Task& Task::operator=(Task&& other) noexcept {
    Terminate(TaskCancellationReason::kAbandoned);
    TaskBase::operator=(std::move(other));
    UASSERT(!other.IsValid());
    return *this;
}

Task::~Task() { Terminate(TaskCancellationReason::kAbandoned); }

impl::ContextAccessor* Task::TryGetContextAccessor() noexcept { return IsValid() ? &GetContext() : nullptr; }

void DetachUnscopedUnsafe(Task&& task) {
    if (task.IsValid()) {
        UASSERT(task.GetContext().UseCount() > 0);
        // If Adopt throws, the Task is kept in a consistent state
        task.GetContext().GetTaskProcessor().Adopt(task.GetContext());
        task.TaskBase::operator=(Task{});
    }
}

}  // namespace engine

USERVER_NAMESPACE_END
