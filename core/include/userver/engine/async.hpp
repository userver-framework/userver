#pragma once

/// @file userver/engine/async.hpp
/// @brief TaskWithResult creation helpers

#include <userver/engine/deadline.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utils/wrapped_call.hpp>

namespace engine::impl {

template <typename Function, typename... Args>
auto MakeTaskWithResult(TaskProcessor& task_processor,
                        Task::Importance importance, Deadline deadline,
                        Function&& f, Args&&... args);

/// Runs an asynchronous function call using specified task processor
template <typename Function, typename... Args>
[[nodiscard]] auto Async(TaskProcessor& task_processor, Function&& f,
                         Args&&... args) {
  return impl::MakeTaskWithResult(task_processor, Task::Importance::kNormal, {},
                                  std::forward<Function>(f),
                                  std::forward<Args>(args)...);
}

/// Runs an asynchronous function call with deadline using specified task
/// processor
template <typename Function, typename... Args>
[[nodiscard]] auto Async(TaskProcessor& task_processor, Deadline deadline,
                         Function&& f, Args&&... args) {
  return impl::MakeTaskWithResult(task_processor, Task::Importance::kNormal,
                                  deadline, std::forward<Function>(f),
                                  std::forward<Args>(args)...);
}

/// Runs an asynchronous function call using task processor of the caller
template <typename Function, typename... Args>
[[nodiscard]] auto Async(Function&& f, Args&&... args) {
  return Async(current_task::GetTaskProcessor(), std::forward<Function>(f),
               std::forward<Args>(args)...);
}

/// Runs an asynchronous function call with deadline using task processor of the
/// caller
template <typename Function, typename... Args>
[[nodiscard]] auto Async(Deadline deadline, Function&& f, Args&&... args) {
  return Async(current_task::GetTaskProcessor(), deadline,
               std::forward<Function>(f), std::forward<Args>(args)...);
}

/// @brief Runs an asynchronous function call that must not be cancelled
/// due to overload using specified task processor
template <typename Function, typename... Args>
[[nodiscard]] auto CriticalAsync(TaskProcessor& task_processor, Function&& f,
                                 Args&&... args) {
  return impl::MakeTaskWithResult(task_processor, Task::Importance::kCritical,
                                  {}, std::forward<Function>(f),
                                  std::forward<Args>(args)...);
}

/// @brief Runs an asynchronous function call that must not be cancelled
/// due to overload using task processor of the caller
template <typename Function, typename... Args>
[[nodiscard]] auto CriticalAsync(Function&& f, Args&&... args) {
  return CriticalAsync(current_task::GetTaskProcessor(),
                       std::forward<Function>(f), std::forward<Args>(args)...);
}

template <typename Function, typename... Args>
auto MakeTaskWithResult(TaskProcessor& task_processor,
                        Task::Importance importance, Deadline deadline,
                        Function&& f, Args&&... args) {
  auto wrapped_call_ptr =
      utils::WrapCall(std::forward<Function>(f), std::forward<Args>(args)...);
  using ResultType = decltype(wrapped_call_ptr->Retrieve());
  return TaskWithResult<ResultType>(task_processor, importance, deadline,
                                    std::move(wrapped_call_ptr));
}

}  // namespace engine::impl
