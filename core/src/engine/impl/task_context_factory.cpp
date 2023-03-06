#include <userver/engine/impl/task_context_factory.hpp>

#include <engine/task/task_context.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

std::size_t GetTaskContextSize() noexcept { return sizeof(TaskContext); }

static_assert(kTaskContextAlignment >= alignof(TaskContext));
static_assert(sizeof(TaskContext) % kTaskContextAlignment == 0);

TaskContext& PlacementNewTaskContext(std::byte* storage, TaskConfig config,
                                     utils::impl::WrappedCallBase& payload) {
  return *new (storage) TaskContext{config.task_processor, config.importance,
                                    config.wait_mode, config.deadline, payload};
}

std::byte* AllocateFusedTaskContext(std::size_t total_size) {
  return static_cast<std::byte*>(
      ::operator new[](total_size, std::align_val_t{kTaskContextAlignment}));
}

void DeleteFusedTaskContext(std::byte* storage) noexcept {
  UASSERT(storage);
  ::operator delete[](storage, std::align_val_t{kTaskContextAlignment});
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
