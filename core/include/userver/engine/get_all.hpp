#pragma once

/// @file userver/engine/get_all.hpp
/// @brief Provides engine::GetAll

#include <type_traits>
#include <vector>

#include <userver/engine/wait_all_checked.hpp>
#include <userver/utils/meta.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @ingroup userver_concurrency
///
/// @brief Waits for the successful completion of all of the specified tasks
/// or the cancellation of the caller.
///
/// Effectively performs `for (auto& task : tasks) task.Get();` with a twist:
/// task.Get() is called in tasks completion order rather than in provided
/// order, thus exceptions are rethrown ASAP.
///
/// After successful return from this method the tasks are invalid,
/// in case of an exception being thrown some of the tasks might be invalid.
///
/// @param tasks either a single container, or a pack of future-like elements.
/// @returns `std::vector<Result>` or `void`, depending on the tasks result type
/// (which must be the same for all `tasks`).
/// @throws WaitInterruptedException when `current_task::IsCancelRequested()`
/// and no TaskCancellationBlockers are present.
/// @throws std::exception rethrows one of specified tasks exception, if any, in
/// no particular order.
///
/// @note Has overall computational complexity of O(N^2),
/// where N is the number of tasks.
/// @note Prefer engine::WaitAllChecked for tasks with a result, unless you
/// specifically need the results stored in a `std::vector` or when storing the
/// results long-term.
template <typename... Tasks>
auto GetAll(Tasks&... tasks);

namespace impl {

template <typename Container>
auto GetAllResultsFromContainer(Container& tasks) {
  using Result = decltype(std::begin(tasks)->Get());

  if constexpr (std::is_void_v<Result>) {
    for (auto& task : tasks) {
      task.Get();
    }
    return;
  } else {
    std::vector<Result> results;
    results.reserve(std::size(tasks));
    for (auto& task : tasks) {
      results.push_back(task.Get());
    }
    return results;
  }
}

template <typename... Tasks>
auto GetAllResultsFromTasks(Tasks&... tasks) {
  using Result = decltype((void(), ..., tasks.Get()));

  if constexpr (std::is_void_v<Result>) {
    static_assert((true && ... && std::is_void_v<decltype(tasks.Get())>));
    (tasks.Get(), ...);
    return;
  } else {
    std::vector<Result> results;
    results.reserve(sizeof...(tasks));
    (results.push_back(tasks.Get()), ...);
    return results;
  }
}

}  // namespace impl

template <typename... Tasks>
auto GetAll(Tasks&... tasks) {
  engine::WaitAllChecked(tasks...);
  if constexpr (meta::impl::IsSingleRange<Tasks...>()) {
    return impl::GetAllResultsFromContainer(tasks...);
  } else {
    return impl::GetAllResultsFromTasks(tasks...);
  }
}

}  // namespace engine

USERVER_NAMESPACE_END
