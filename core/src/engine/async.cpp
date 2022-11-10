#include <userver/engine/async.hpp>

#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

size_t TaskFactory::GetTaskContextSize() {
  return sizeof(TaskContext);
}

size_t TaskFactory::GetTaskContextAlignment() {
  return alignof(TaskContext);
}

}

USERVER_NAMESPACE_END
