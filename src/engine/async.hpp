#pragma once

#include <engine/task/task.hpp>
#include <engine/task/task_processor.hpp>

#include "async_task.hpp"
#include "future.hpp"

namespace engine {

template <typename Function, typename... Args>
auto Async(TaskProcessor& task_processor, Function&& f, Args&&... args) {
  using Ret = decltype(f(std::forward<Args>(args)...));

  Promise<Ret> promise;
  auto future = promise.GetFuture();
  new AsyncTask<Ret>(task_processor, std::move(promise),
                     std::forward<Function>(f), std::forward<Args>(args)...);
  return future;
}

template <typename Function, typename... Args>
auto Async(Function&& f, Args&&... args) {
  return Async(CurrentTask::GetTaskProcessor(), std::forward<Function>(f),
               std::forward<Args>(args)...);
}

}  // namespace engine
