#include <userver/engine/async.hpp>

#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

size_t TaskFactory::GetTaskContextSize() {
  return sizeof(TaskContext);
}

}

USERVER_NAMESPACE_END
