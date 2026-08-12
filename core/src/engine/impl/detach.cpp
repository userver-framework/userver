#include <userver/engine/impl/detach.hpp>

#include <utility>

#include <userver/utils/impl/internal_tag.hpp>

#include <engine/impl/task_context_accessor.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

void DetachUnscopedUnsafeNoCancellationOnShutdown(utils::impl::InternalTag, TaskBase&& task) noexcept {
    // Dropping the only owning reference: the task keeps running on its own,
    // kept alive by the engine reference baton until it finishes.
    //
    // TaskProcessor awaits the destruction of all remaining TaskContext instances using TaskCounter.
    [[maybe_unused]] const auto context = TaskContextAccessor::ExtractContext(std::move(task));
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
