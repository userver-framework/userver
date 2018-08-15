#pragma once

#include <engine/async_task.hpp>
#include <engine/task/task_processor.hpp>
#include <engine/wrapped_call.hpp>

namespace engine {
namespace impl {

template <typename Function, typename... Args>
auto MakeAsync(TaskProcessor& task_processor, Task::Importance importance,
               Function&& f, Args&&... args);
}

template <typename Function, typename... Args>
__attribute__((warn_unused_result)) auto Async(TaskProcessor& task_processor,
                                               Function&& f, Args&&... args) {
  return impl::MakeAsync(task_processor, Task::Importance::kNormal,
                         std::forward<Function>(f),
                         std::forward<Args>(args)...);
}

template <typename Function, typename... Args>
__attribute__((warn_unused_result)) auto Async(Function&& f, Args&&... args) {
  return Async(current_task::GetTaskProcessor(), std::forward<Function>(f),
               std::forward<Args>(args)...);
}

template <typename Function, typename... Args>
__attribute__((warn_unused_result)) auto CriticalAsync(
    TaskProcessor& task_processor, Function&& f, Args&&... args) {
  return impl::MakeAsync(task_processor, Task::Importance::kCritical,
                         std::forward<Function>(f),
                         std::forward<Args>(args)...);
}

template <typename Function, typename... Args>
__attribute__((warn_unused_result)) auto CriticalAsync(Function&& f,
                                                       Args&&... args) {
  return CriticalAsync(current_task::GetTaskProcessor(),
                       std::forward<Function>(f), std::forward<Args>(args)...);
}

namespace impl {

template <typename Function, typename... Args>
auto MakeAsync(TaskProcessor& task_processor, Task::Importance importance,
               Function&& f, Args&&... args) {
  auto wrapped_call_ptr =
      impl::WrapCall(std::forward<Function>(f), std::forward<Args>(args)...);
  using ResultType = decltype(wrapped_call_ptr->Retrieve());
  return AsyncTask<ResultType>(task_processor, importance,
                               std::move(wrapped_call_ptr));
}

}  // namespace impl
}  // namespace engine
