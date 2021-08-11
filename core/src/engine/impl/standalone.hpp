#pragma once

/// @file engine/standalone.hpp
/// @brief Standalone TaskProcessor support
///
/// For internal use only.
///
/// Standalone task processors should not be used with components.
///
/// You should never have more than one instance of TaskProcessorPools in your
/// applications. Otherwise you may experience spurious lockups.

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include <userver/engine/run_standalone.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>

namespace engine::impl {

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

void RunOnTaskProcessorSync(TaskProcessor& tp, std::function<void()> user_cb);

}  // namespace engine::impl
