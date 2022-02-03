#pragma once

/// @file userver/engine/wait_any.hpp

#include <chrono>
#include <optional>
#include <vector>

#include <userver/engine/deadline.hpp>
#include <userver/engine/task/task.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @brief Waits for the completion of any of the specified tasks or the
/// cancellation of the caller.
/// Returns the index of the completed task, or `std::nullopt` if there are no
/// completed task (possible if current task was cancelled).
template <typename Container>
std::optional<size_t> WaitAny(Container& tasks);

/// @brief Waits for the completion of any of the specified tasks or the
/// cancellation of the caller.
/// Returns the index of the completed task, or `std::nullopt` if there are no
/// completed task (possible if current task was cancelled).
template <typename... Tasks>
std::optional<size_t> WaitAny(Tasks&... tasks);

/// @brief Waits for the completion of any of the specified tasks or the
/// cancellation of the caller within the specified time.
/// Returns the index of the completed task, or `std::nullopt` if there are no
/// completed task.
template <typename Container, typename Rep, typename Period>
std::optional<size_t> WaitAnyFor(
    const std::chrono::duration<Rep, Period>& duration, Container& tasks);

/// @brief Waits for the completion of any of the specified tasks or the
/// cancellation of the caller within the specified time.
/// Returns the index of the completed task, or `std::nullopt` if there are no
/// completed task.
template <typename... Tasks, typename Rep, typename Period>
std::optional<size_t> WaitAnyFor(
    const std::chrono::duration<Rep, Period>& duration, Tasks&... tasks);

/// @brief Waits for the completion of any of the specified tasks or the
/// cancellation of the caller before the specified time is reached.
/// Returns the index of the completed task, or `std::nullopt` if there are no
/// completed task.
template <typename Container, typename Clock, typename Duration>
std::optional<size_t> WaitAnyUntil(
    const std::chrono::time_point<Clock, Duration>& until, Container& tasks);

/// @brief Waits for the completion of any of the specified tasks or the
/// cancellation of the caller before the specified time is reached.
/// Returns the index of the completed task, or `std::nullopt` if there are no
/// completed task.
template <typename... Tasks, typename Clock, typename Duration>
std::optional<size_t> WaitAnyUntil(
    const std::chrono::time_point<Clock, Duration>& until, Tasks&... tasks);

/// @brief Waits for the completion of any of the specified tasks or the
/// cancellation of the caller before the specified time is reached.
/// Returns the index of the completed task, or `std::nullopt` if there are no
/// completed task.
template <typename Container>
std::optional<size_t> WaitAnyUntil(Deadline, Container& tasks);

/// @brief Waits for the completion of any of the specified tasks or the
/// cancellation of the caller before the specified time is reached.
/// Returns the index of the completed task, or `std::nullopt` if there are no
/// completed task.
template <typename... Tasks>
std::optional<size_t> WaitAnyUntil(Deadline, Tasks&... tasks);

template <typename Container>
std::optional<size_t> WaitAny(Container& tasks) {
  return WaitAnyUntil({}, tasks);
}

template <typename... Tasks>
std::optional<size_t> WaitAny(Tasks&... tasks) {
  return WaitAnyUntil({}, tasks...);
}

template <typename Container, typename Rep, typename Period>
std::optional<size_t> WaitAnyFor(
    const std::chrono::duration<Rep, Period>& duration, Container& tasks) {
  return WaitAnyUntil(Deadline::FromDuration(duration), tasks);
}

template <typename... Tasks, typename Rep, typename Period>
std::optional<size_t> WaitAnyFor(
    const std::chrono::duration<Rep, Period>& duration, Tasks&... tasks) {
  return WaitAnyUntil(Deadline::FromDuration(duration), tasks...);
}

template <typename Container, typename Clock, typename Duration>
std::optional<size_t> WaitAnyUntil(
    const std::chrono::time_point<Clock, Duration>& until, Container& tasks) {
  return WaitAnyUntil(Deadline::FromTimePoint(until), tasks);
}

template <typename... Tasks, typename Clock, typename Duration>
std::optional<size_t> WaitAnyUntil(
    const std::chrono::time_point<Clock, Duration>& until, Tasks&... tasks) {
  return WaitAnyUntil(Deadline::FromTimePoint(until), tasks...);
}

namespace impl {

struct IndexedWaitAnyElement final {
  IndexedWaitAnyElement(Task::ContextAccessor context_accessor, size_t index)
      : context_accessor(context_accessor), index(index) {}

  Task::ContextAccessor context_accessor;
  size_t index;
};

class WaitAnyHelper final {
 public:
  template <typename Container>
  static std::optional<size_t> WaitAnyUntil(Deadline deadline,
                                            Container& tasks) {
    auto iwa_elements = MakeIwaElementsFromContainer(tasks);
    return DoWaitAnyUntil(iwa_elements, deadline);
  }

  template <typename... Tasks>
  static std::optional<size_t> WaitAnyUntil(Deadline deadline,
                                            Tasks&... tasks) {
    auto iwa_elements = MakeIwaElementsFromTasks(tasks...);
    return DoWaitAnyUntil(iwa_elements, deadline);
  }

 private:
  template <typename Container>
  static std::vector<IndexedWaitAnyElement> MakeIwaElementsFromContainer(
      Container& tasks);

  template <typename... Tasks>
  static std::vector<IndexedWaitAnyElement> MakeIwaElementsFromTasks(
      const Tasks&... tasks);

  static std::optional<size_t> DoWaitAnyUntil(
      std::vector<IndexedWaitAnyElement>& iwa_elements, Deadline deadline);
};

template <typename Container>
std::vector<IndexedWaitAnyElement> WaitAnyHelper::MakeIwaElementsFromContainer(
    Container& tasks) {
  if constexpr (std::is_base_of_v<Task, Container>) {
    return MakeIwaElementsFromTasks(tasks);
  } else {
    std::vector<IndexedWaitAnyElement> iwa_elements;
    static_assert(
        std::is_base_of_v<
            Task, std::remove_reference_t<decltype(*std::begin(tasks))>>,
        "Calling WaitAny for objects of type not derived from engine::Task");
    iwa_elements.reserve(std::size(tasks));
    for (size_t i = 0; i < std::size(tasks); i++) {
      auto& task = tasks[i];
      UINVARIANT(!task.IsSharedWaitAllowed(),
                 "WaitAny does not support SharedTaskWithResult");

      if (task.IsValid())
        iwa_elements.emplace_back(task.GetContextAccessor(), i);
    }
    return iwa_elements;
  }
}

template <typename... Tasks>
std::vector<IndexedWaitAnyElement> WaitAnyHelper::MakeIwaElementsFromTasks(
    const Tasks&... tasks) {
  static_assert(
      std::conjunction_v<std::is_base_of<Task, Tasks>...>,
      "Calling WaitAny for objects of type not derived from engine::Task");
  std::vector<IndexedWaitAnyElement> iwa_elements;
  iwa_elements.reserve(sizeof...(Tasks));

  [[maybe_unused]] const auto add_task_if_valid = [&](const auto& task,
                                                      size_t idx) {
    UINVARIANT(!task.IsSharedWaitAllowed(),
               "WaitAny does not support SharedTaskWithResult");
    if (task.IsValid())
      iwa_elements.emplace_back(task.GetContextAccessor(), idx);
  };
  std::size_t index = 0;
  (add_task_if_valid(tasks, index++), ...);
  return iwa_elements;
}

}  // namespace impl

template <typename Container>
std::optional<size_t> WaitAnyUntil(Deadline deadline, Container& tasks) {
  return impl::WaitAnyHelper::WaitAnyUntil(deadline, tasks);
}

template <typename... Tasks>
std::optional<size_t> WaitAnyUntil(Deadline deadline, Tasks&... tasks) {
  return impl::WaitAnyHelper::WaitAnyUntil(deadline, tasks...);
}

}  // namespace engine

USERVER_NAMESPACE_END
