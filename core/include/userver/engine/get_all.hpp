#pragma once

/// @file userver/engine/get_all.hpp

#include <vector>

#include <userver/engine/task/task.hpp>
#include <userver/engine/task/task_with_result.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @brief Waits for the successful completion of all of the specified tasks
/// or the cancellation of the caller.
/// Effectively performs `for (auto& task : tasks) task.Get();` with a twist:
/// task.Get() is called in tasks completion order rather than in provided
/// order, thus exceptions are rethrown ASAP.
/// After successful return from this method the tasks are invalid,
/// in case of exception being thrown some of the tasks might be invalid.
///
/// @throws WaitInterruptedException when `current_task::IsCancelRequested()`
/// and no TaskCancellationBlockers are present.
/// Rethrows one of specified tasks exception, if any, in no particular order.
///
/// @note: has overall computational complexity of O(N^2),
/// where N is the number of tasks.
void GetAll(std::vector<TaskWithResult<void>>& tasks);

/// @brief Waits for the successful completion of all of the specified tasks
/// or the cancellation of the caller.
/// Effectively performs `for (auto& task : tasks) task.Get();` with a twist:
/// task.Get() is called in tasks completion order rather than in provided
/// order, thus exceptions are rethrown ASAP.
/// After successful return from this method the tasks are invalid,
/// in case of exception being thrown some of the tasks might be invalid.
///
/// @throws WaitInterruptedException when `current_task::IsCancelRequested()`
/// and no TaskCancellationBlockers are present.
/// Rethrows one of specified tasks exception, if any, in no particular order.
///
/// @note: has overall computational complexity of O(N^2),
/// where N is the number of tasks.
template <typename... Tasks>
void GetAll(Tasks&... tasks);

namespace impl {

class GetAllElement final {
 public:
  explicit GetAllElement(TaskWithResult<void>& task);

  void Access();

  bool WasAccessed() const;

  bool IsTaskFinished() const;

  ContextAccessor* TryGetContextAccessor();

 private:
  TaskWithResult<void>& task_;
  bool was_accessed_{false};
};

class GetAllHelper final {
 public:
  using Container = std::vector<TaskWithResult<void>>;

  static void GetAll(Container& tasks);

  template <typename... Tasks>
  static void GetAll(Tasks&... tasks) {
    auto ga_elements = BuildGetAllElementsFromTasks(tasks...);
    return DoGetAll(ga_elements);
  }

 private:
  static std::vector<GetAllElement> BuildGetAllElementsFromContainer(
      Container& tasks);

  template <typename... Tasks>
  static std::vector<GetAllElement> BuildGetAllElementsFromTasks(
      Tasks&... tasks);

  static void DoGetAll(std::vector<GetAllElement>& ga_elements);
};

template <typename... Tasks>
std::vector<GetAllElement> GetAllHelper::BuildGetAllElementsFromTasks(
    Tasks&... tasks) {
  static_assert(
      std::conjunction_v<std::is_same<TaskWithResult<void>, Tasks>...>,
      "Calling GetAll for objects of type different from "
      "engine::TaskWithResult<void>");

  std::vector<GetAllElement> ga_elements;
  ga_elements.reserve(sizeof...(Tasks));

  auto emplace_ga_element = [&ga_elements](auto& task) {
    task.EnsureValid();
    ga_elements.emplace_back(task);
  };
  (emplace_ga_element(tasks), ...);

  return ga_elements;
}

}  // namespace impl

template <typename... Tasks>
void GetAll(Tasks&... tasks) {
  impl::GetAllHelper::GetAll(tasks...);
}

}  // namespace engine

USERVER_NAMESPACE_END
