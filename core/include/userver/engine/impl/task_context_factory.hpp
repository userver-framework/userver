#pragma once

#include <cstddef>
#include <memory>
#include <new>
#include <utility>

#include <userver/engine/impl/task_context_holder.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/impl/wrapped_call.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

std::size_t GetTaskContextSize() noexcept;

// cpp contains a static_assert that this matches the actual alignment.
inline constexpr std::size_t kTaskContextAlignment = 16;

struct TaskConfig final {
  engine::TaskProcessor& task_processor;
  Task::Importance importance{Task::Importance::kNormal};
  Task::WaitMode wait_mode{Task::WaitMode::kSingleWaiter};
  engine::Deadline deadline;
};

[[nodiscard]] TaskContext& PlacementNewTaskContext(
    std::byte* storage, TaskConfig config,
    utils::impl::WrappedCallBase& payload);

// Never returns nullptr, may throw.
std::byte* AllocateFusedTaskContext(std::size_t total_size);

void DeleteFusedTaskContext(std::byte* storage) noexcept;

// The allocations for TaskContext and WrappedCall are manually fused. The
// layout is as follows:
// 1. TaskContext, guaranteed to be at the beginning of the allocation
// 2. WrappedCall
//
// The whole allocation, as well as the lifetimes of the objects inside, are
// managed by boost::intrusive_ptr<TaskContext> through intrusive_ptr_add_ref
// and intrusive_ptr_release hooks.
template <typename Function, typename... Args>
TaskContextHolder MakeTask(TaskConfig config, Function&& f, Args&&... args) {
  using WrappedCallType = utils::impl::WrappedCallImplType<Function, Args...>;

  constexpr auto kPayloadSize = sizeof(WrappedCallType);
  constexpr auto kPayloadAlignment = alignof(WrappedCallType);
  // Makes sure that the WrappedCall is aligned appropriately just by being
  // placed after TaskContext. To remove this limitation, we could store
  // the dynamic alignment somewhere and use it in DeleteFusedTaskContext.
  static_assert(kPayloadAlignment <= kTaskContextAlignment);

  const auto task_context_size = GetTaskContextSize();
  const auto total_size = task_context_size + kPayloadSize;

  std::byte* const storage = AllocateFusedTaskContext(total_size);
  utils::FastScopeGuard delete_guard{
      [&]() noexcept { DeleteFusedTaskContext(storage); }};

  std::byte* const payload_storage = storage + task_context_size;

  auto& payload = utils::impl::PlacementNewWrapCall(
      payload_storage, std::forward<Function>(f), std::forward<Args>(args)...);
  utils::FastScopeGuard destroy_payload_guard{
      [&]() noexcept { std::destroy_at(&payload); }};

  auto& context = PlacementNewTaskContext(storage, config, payload);

  destroy_payload_guard.Release();
  delete_guard.Release();
  return TaskContextHolder::Adopt(context);
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
