#pragma once

/// @file userver/engine/wait_all_checked.hpp
/// @brief Provides engine::WaitAllChecked

#include <chrono>
#include <vector>

#include <userver/engine/deadline.hpp>
#include <userver/engine/future_status.hpp>
#include <userver/utils/impl/span.hpp>
#include <userver/utils/meta.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @ingroup userver_concurrency
///
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
/// @throws WaitInterruptedException when `current_task::ShouldCancel()` (for
/// WaitAllChecked versions without a deadline)
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

/// @ingroup userver_concurrency
///
/// @overload void WaitAllChecked(Tasks&... tasks)
template <typename... Tasks, typename Rep, typename Period>
[[nodiscard]] FutureStatus WaitAllCheckedFor(
    const std::chrono::duration<Rep, Period>& duration, Tasks&... tasks);

/// @ingroup userver_concurrency
///
/// @overload void WaitAllChecked(Tasks&... tasks)
template <typename... Tasks, typename Clock, typename Duration>
[[nodiscard]] FutureStatus WaitAllCheckedUntil(
    const std::chrono::time_point<Clock, Duration>& until, Tasks&... tasks);

/// @ingroup userver_concurrency
///
/// @overload void WaitAllChecked(Tasks&... tasks)
template <typename... Tasks>
[[nodiscard]] FutureStatus WaitAllCheckedUntil(Deadline deadline,
                                               Tasks&... tasks);

namespace impl {

class ContextAccessor;

FutureStatus DoWaitAllChecked(utils::impl::Span<ContextAccessor*> targets,
                              Deadline deadline);

template <typename Container>
FutureStatus WaitAllCheckedFromContainer(Deadline deadline, Container& tasks) {
  std::vector<ContextAccessor*> targets;
  targets.reserve(std::size(tasks));

  for (auto& task : tasks) {
    targets.push_back(task.TryGetContextAccessor());
  }

  return impl::DoWaitAllChecked(targets, deadline);
}

template <typename... Tasks>
FutureStatus WaitAllCheckedFromTasks(Deadline deadline, Tasks&... tasks) {
  ContextAccessor* targets[]{tasks.TryGetContextAccessor()...};
  return impl::DoWaitAllChecked(targets, deadline);
}

inline FutureStatus WaitAllCheckedFromTasks(Deadline /*deadline*/) {
  return FutureStatus::kReady;
}

void HandleWaitAllStatus(FutureStatus status);

}  // namespace impl

template <typename... Tasks>
void WaitAllChecked(Tasks&... tasks) {
  impl::HandleWaitAllStatus(engine::WaitAllCheckedUntil(Deadline{}, tasks...));
}

template <typename... Tasks, typename Rep, typename Period>
[[nodiscard]] FutureStatus WaitAllCheckedFor(
    const std::chrono::duration<Rep, Period>& duration, Tasks&... tasks) {
  return WaitAllCheckedUntil(Deadline::FromDuration(duration), tasks...);
}

template <typename... Tasks, typename Clock, typename Duration>
[[nodiscard]] FutureStatus WaitAllCheckedUntil(
    const std::chrono::time_point<Clock, Duration>& until, Tasks&... tasks) {
  return WaitAllCheckedUntil(Deadline::FromTimePoint(until), tasks...);
}

template <typename... Tasks>
[[nodiscard]] FutureStatus WaitAllCheckedUntil(Deadline deadline,
                                               Tasks&... tasks) {
  if constexpr (meta::impl::IsSingleRange<Tasks...>()) {
    return impl::WaitAllCheckedFromContainer(deadline, tasks...);
  } else {
    return impl::WaitAllCheckedFromTasks(deadline, tasks...);
  }
}

}  // namespace engine

USERVER_NAMESPACE_END
