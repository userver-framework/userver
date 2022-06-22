#pragma once

/// @file userver/engine/wait_all_checked.hpp
/// @brief Provides engine::WaitAllChecked

#include <vector>

#include <userver/utils/impl/span.hpp>
#include <userver/utils/meta.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @brief Waits for the successful completion of all of the specified tasks
/// or for the cancellation of the caller.
///
/// Effectively performs `for (auto& task : tasks) task.Wait();` with a twist:
/// if any task completes with an exception, it gets rethrown ASAP.
///
/// Invalid tasks are skipped.
///
/// Tasks are not invalidated by `WaitAllChecked`; the result can be retrieved
/// after the call.
///
/// @param tasks either a single container, or a pack of future-like elements.
/// @throws WaitInterruptedException when `current_task::IsCancelRequested()`
/// and no TaskCancellationBlockers are present.
/// @throws std::exception one of specified tasks exception, if any, in no
/// particular order.
///
/// @note Has overall computational complexity of O(N^2),
/// where N is the number of tasks.
/// @note Keeping the tasks valid may have a small extra memory impact. Make
/// sure to drop the tasks after reading the results.
/// @note Prefer engine::GetAll for tasks without a result, unless you
/// specifically need to keep the tasks alive for some reason.
template <typename... Tasks>
void WaitAllChecked(Tasks&... tasks);

namespace impl {

class ContextAccessor;

void DoWaitAllChecked(utils::impl::Span<ContextAccessor*> targets);

template <typename Container>
void WaitAllCheckedFromContainer(Container& tasks) {
  std::vector<ContextAccessor*> targets;
  targets.reserve(std::size(tasks));

  for (auto& task : tasks) {
    targets.push_back(task.TryGetContextAccessor());
  }

  DoWaitAllChecked(targets);
}

template <typename... Tasks>
void WaitAllCheckedFromTasks(Tasks&... tasks) {
  ContextAccessor* targets[]{tasks.TryGetContextAccessor()...};
  DoWaitAllChecked(targets);
}

inline void WaitAllCheckedFromTasks() {}

}  // namespace impl

template <typename... Tasks>
void WaitAllChecked(Tasks&... tasks) {
  if constexpr (meta::impl::IsSingleRange<Tasks...>()) {
    return impl::WaitAllCheckedFromContainer(tasks...);
  } else {
    return impl::WaitAllCheckedFromTasks(tasks...);
  }
}

}  // namespace engine

USERVER_NAMESPACE_END
