#pragma once

/// @file engine/standalone.hpp
/// @brief Standalone TaskProcessor support
///
/// This header is intended for very limited use. One should consult someone
/// from common components development group before considering its inclusion.
///
/// Standalone task processors should not be used with components.
///
/// You should never have more than one instance of TaskProcessorPools in your
/// applications. Otherwise you may experience spurious lockups.

#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace engine {

class TaskProcessor;

namespace impl {

struct TaskProcessorPoolsConfig {
  std::optional<size_t> initial_coro_pool_size;
  std::optional<size_t> max_coro_pool_size;
  std::optional<size_t> ev_threads_num;
  std::optional<std::string> ev_thread_name;
};

class TaskProcessorPools;

std::shared_ptr<TaskProcessorPools> MakeTaskProcessorPools(
    const TaskProcessorPoolsConfig& = {});

class TaskProcessorHolder final {
 public:
  static TaskProcessorHolder MakeTaskProcessor(
      size_t threads_num, std::string thread_name,
      std::shared_ptr<TaskProcessorPools> pools);

  explicit TaskProcessorHolder(std::unique_ptr<TaskProcessor>&&);
  ~TaskProcessorHolder();

  TaskProcessorHolder(const TaskProcessorHolder&) = delete;
  TaskProcessorHolder(TaskProcessorHolder&&) noexcept;
  TaskProcessorHolder& operator=(const TaskProcessorHolder&) = delete;
  TaskProcessorHolder& operator=(TaskProcessorHolder&&) noexcept;

  TaskProcessor* Get() { return task_processor_.get(); }

  TaskProcessor& operator*() { return *task_processor_; }
  TaskProcessor* operator->() { return Get(); }

 private:
  std::unique_ptr<TaskProcessor> task_processor_;
};

class TaskContext;

void RunOnTaskProcessorSync(TaskProcessor& tp, std::function<void()> user_cb);

}  // namespace impl

namespace current_task {
impl::TaskContext* GetCurrentTaskContext();
}

}  // namespace engine
