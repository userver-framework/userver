#pragma once

/// @file userver/engine/wait_any.hpp
/// @brief Provides engine::WaitAny, engine::WaitAnyFor and engine::WaitAnyUntil

#include <chrono>
#include <optional>
#include <vector>

#include <userver/engine/deadline.hpp>
#include <userver/utils/impl/span.hpp>
#include <userver/utils/meta.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @ingroup userver_concurrency
///
/// @brief Waits for the completion of any of the specified tasks or the
/// cancellation of the caller.
///
/// Could be used to get the ready HTTP requests ASAP:
/// @snippet src/clients/http/client_wait_test.cpp HTTP Client - waitany
///
/// Works with different types of tasks and futures:
/// @snippet src/engine/wait_any_test.cpp sample waitany
///
/// @param tasks either a single container, or a pack of future-like elements.
/// @returns the index of the completed task, or `std::nullopt` if there are no
/// completed tasks (possible if current task was cancelled).
template <typename... Tasks>
std::optional<std::size_t> WaitAny(Tasks&... tasks);

/// @ingroup userver_concurrency
///
/// @overload std::optional<std::size_t> WaitAny(Tasks&... tasks)
template <typename... Tasks, typename Rep, typename Period>
std::optional<std::size_t> WaitAnyFor(
    const std::chrono::duration<Rep, Period>& duration, Tasks&... tasks);

/// @ingroup userver_concurrency
///
/// @overload std::optional<std::size_t> WaitAny(Tasks&... tasks)
template <typename... Tasks, typename Clock, typename Duration>
std::optional<std::size_t> WaitAnyUntil(
    const std::chrono::time_point<Clock, Duration>& until, Tasks&... tasks);

/// @ingroup userver_concurrency
///
/// @overload std::optional<std::size_t> WaitAny(Tasks&... tasks)
template <typename... Tasks>
std::optional<std::size_t> WaitAnyUntil(Deadline, Tasks&... tasks);

template <typename... Tasks>
std::optional<std::size_t> WaitAny(Tasks&... tasks) {
  return engine::WaitAnyUntil(Deadline{}, tasks...);
}

template <typename... Tasks, typename Rep, typename Period>
std::optional<std::size_t> WaitAnyFor(
    const std::chrono::duration<Rep, Period>& duration, Tasks&... tasks) {
  return engine::WaitAnyUntil(Deadline::FromDuration(duration), tasks...);
}

template <typename... Tasks, typename Clock, typename Duration>
std::optional<std::size_t> WaitAnyUntil(
    const std::chrono::time_point<Clock, Duration>& until, Tasks&... tasks) {
  return engine::WaitAnyUntil(Deadline::FromTimePoint(until), tasks...);
}

namespace impl {

class ContextAccessor;

std::optional<std::size_t> DoWaitAny(
    utils::impl::Span<ContextAccessor*> targets, Deadline deadline);

template <typename Container>
std::optional<std::size_t> WaitAnyFromContainer(Deadline deadline,
                                                Container& tasks) {
  const auto size = std::size(tasks);
  std::vector<ContextAccessor*> targets;
  targets.reserve(size);

  for (auto& task : tasks) {
    targets.push_back(task.TryGetContextAccessor());
  }

  return DoWaitAny(targets, deadline);
}

template <typename... Tasks>
std::optional<std::size_t> WaitAnyFromTasks(Deadline deadline,
                                            Tasks&... tasks) {
  ContextAccessor* wa_elements[]{tasks.TryGetContextAccessor()...};
  return DoWaitAny(wa_elements, deadline);
}

inline std::optional<std::size_t> WaitAnyFromTasks(Deadline) { return {}; }

}  // namespace impl

template <typename... Tasks>
std::optional<std::size_t> WaitAnyUntil(Deadline deadline, Tasks&... tasks) {
  if constexpr (meta::impl::IsSingleRange<Tasks...>()) {
    return impl::WaitAnyFromContainer(deadline, tasks...);
  } else {
    return impl::WaitAnyFromTasks(deadline, tasks...);
  }
}

}  // namespace engine

USERVER_NAMESPACE_END
