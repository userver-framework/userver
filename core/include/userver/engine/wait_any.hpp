#pragma once

/// @file userver/engine/wait_any.hpp
/// @brief Provides engine::WaitAny, engine::WaitAnyFor and engine::WaitAnyUntil

#include <chrono>
#include <optional>
#include <vector>

#include <userver/engine/deadline.hpp>
#include <userver/engine/task/task.hpp>
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
/// @returns the index of the completed task, or `std::nullopt` if there are no
/// completed tasks (possible if current task was cancelled).
template <typename Container>
std::optional<std::size_t> WaitAny(Container& tasks);

/// @ingroup userver_concurrency
///
/// @overload std::optional<std::size_t> WaitAny(Container& tasks)
template <typename... Tasks>
std::optional<std::size_t> WaitAny(Tasks&... tasks);

/// @ingroup userver_concurrency
///
/// @overload std::optional<std::size_t> WaitAny(Container& tasks)
template <typename Container, typename Rep, typename Period>
std::optional<std::size_t> WaitAnyFor(
    const std::chrono::duration<Rep, Period>& duration, Container& tasks);

/// @ingroup userver_concurrency
///
/// @overload std::optional<std::size_t> WaitAny(Container& tasks)
template <typename... Tasks, typename Rep, typename Period>
std::optional<std::size_t> WaitAnyFor(
    const std::chrono::duration<Rep, Period>& duration, Tasks&... tasks);

/// @ingroup userver_concurrency
///
/// @overload std::optional<std::size_t> WaitAny(Container& tasks)
template <typename Container, typename Clock, typename Duration>
std::optional<std::size_t> WaitAnyUntil(
    const std::chrono::time_point<Clock, Duration>& until, Container& tasks);

/// @ingroup userver_concurrency
///
/// @overload std::optional<std::size_t> WaitAny(Container& tasks)
template <typename... Tasks, typename Clock, typename Duration>
std::optional<std::size_t> WaitAnyUntil(
    const std::chrono::time_point<Clock, Duration>& until, Tasks&... tasks);

/// @ingroup userver_concurrency
///
/// @overload std::optional<std::size_t> WaitAny(Container& tasks)
template <typename Container>
std::optional<std::size_t> WaitAnyUntil(Deadline, Container& tasks);

/// @ingroup userver_concurrency
///
/// @overload std::optional<std::size_t> WaitAny(Container& tasks)
template <typename... Tasks>
std::optional<std::size_t> WaitAnyUntil(Deadline, Tasks&... tasks);

template <typename Container>
std::optional<std::size_t> WaitAny(Container& tasks) {
  return engine::WaitAnyUntil({}, tasks);
}

template <typename... Tasks>
std::optional<std::size_t> WaitAny(Tasks&... tasks) {
  return engine::WaitAnyUntil({}, tasks...);
}

template <typename Container, typename Rep, typename Period>
std::optional<std::size_t> WaitAnyFor(
    const std::chrono::duration<Rep, Period>& duration, Container& tasks) {
  return engine::WaitAnyUntil(Deadline::FromDuration(duration), tasks);
}

template <typename... Tasks, typename Rep, typename Period>
std::optional<std::size_t> WaitAnyFor(
    const std::chrono::duration<Rep, Period>& duration, Tasks&... tasks) {
  return engine::WaitAnyUntil(Deadline::FromDuration(duration), tasks...);
}

template <typename Container, typename Clock, typename Duration>
std::optional<std::size_t> WaitAnyUntil(
    const std::chrono::time_point<Clock, Duration>& until, Container& tasks) {
  return engine::WaitAnyUntil(Deadline::FromTimePoint(until), tasks);
}

template <typename... Tasks, typename Clock, typename Duration>
std::optional<std::size_t> WaitAnyUntil(
    const std::chrono::time_point<Clock, Duration>& until, Tasks&... tasks) {
  return engine::WaitAnyUntil(Deadline::FromTimePoint(until), tasks...);
}

namespace impl {

struct IndexedWaitAnyElement final {
  IndexedWaitAnyElement() = default;
  IndexedWaitAnyElement(ContextAccessor* context_accessor, std::size_t index)
      : context_accessor(context_accessor), index(index) {}

  ContextAccessor* context_accessor = nullptr;
  std::size_t index{};
};

class WaitAnyHelper final {
 public:
  template <typename Container>
  static std::optional<std::size_t> WaitIwaElementsFromContainer(
      Deadline deadline, Container& tasks);

  template <typename... Tasks>
  static std::optional<std::size_t> WaitIwaElementsFromTasks(Deadline deadline,
                                                             Tasks&... tasks);

  static std::optional<std::size_t> WaitIwaElementsFromTasks(
      Deadline /*deadline*/) {
    return {};
  }

 private:
  static std::optional<std::size_t> DoWaitAnyUntil(
      IndexedWaitAnyElement* iwa_elements_begin,
      IndexedWaitAnyElement* iwa_elements_end, Deadline deadline);
};

template <typename Container>
std::optional<std::size_t> WaitAnyHelper::WaitIwaElementsFromContainer(
    Deadline deadline, Container& tasks) {
  const auto size = std::size(tasks);

  std::vector<IndexedWaitAnyElement> iwa_elements;
  iwa_elements.reserve(size);
  for (std::size_t i = 0; i < size; i++) {
    auto& task = tasks[i];
    auto context_acessor = task.TryGetContextAccessor();
    if (context_acessor) {
      iwa_elements.emplace_back(context_acessor, i);
    }
  }

  return DoWaitAnyUntil(iwa_elements.data(),
                        iwa_elements.data() + iwa_elements.size(), deadline);
}

template <typename... Tasks>
std::optional<std::size_t> WaitAnyHelper::WaitIwaElementsFromTasks(
    Deadline deadline, Tasks&... tasks) {
  IndexedWaitAnyElement iwa_elements[sizeof...(Tasks)];
  std::size_t empty_pos = 0;

  [[maybe_unused]] const auto add_task_if_valid = [&](auto& task,
                                                      std::size_t idx) {
    auto context_acessor = task.TryGetContextAccessor();
    if (context_acessor) {
      iwa_elements[empty_pos++] = {context_acessor, idx};
    }
  };

  std::size_t index = 0;
  (add_task_if_valid(tasks, index++), ...);
  return DoWaitAnyUntil(iwa_elements, iwa_elements + empty_pos, deadline);
}

}  // namespace impl

template <typename Container>
std::optional<std::size_t> WaitAnyUntil(Deadline deadline, Container& tasks) {
  if constexpr (!meta::kIsRange<Container>) {
    return impl::WaitAnyHelper::WaitIwaElementsFromTasks(deadline, tasks);
  } else {
    return impl::WaitAnyHelper::WaitIwaElementsFromContainer(deadline, tasks);
  }
}

template <typename... Tasks>
std::optional<std::size_t> WaitAnyUntil(Deadline deadline, Tasks&... tasks) {
  return impl::WaitAnyHelper::WaitIwaElementsFromTasks(deadline, tasks...);
}

}  // namespace engine

USERVER_NAMESPACE_END
