#include <userver/engine/task/task_context_holder.hpp>

#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

size_t GetTaskContextSize() { return sizeof(TaskContext); }

size_t GetTaskContextAlignment() { return alignof(TaskContext); }

}  // namespace engine::impl

USERVER_NAMESPACE_END
