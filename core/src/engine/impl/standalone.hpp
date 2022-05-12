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
#include <string>

#include <userver/engine/run_standalone.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/not_null.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskProcessorPools;

std::shared_ptr<TaskProcessorPools> MakeTaskProcessorPools(
    const TaskProcessorPoolsConfig& pools_config);

class TaskProcessorHolder final {
 public:
  static TaskProcessorHolder Make(std::size_t threads_num,
                                  std::string thread_name,
                                  std::shared_ptr<TaskProcessorPools> pools);

  explicit TaskProcessorHolder(std::unique_ptr<TaskProcessor>&&);

  TaskProcessorHolder(TaskProcessorHolder&&) noexcept = default;
  TaskProcessorHolder& operator=(TaskProcessorHolder&&) noexcept = default;
  ~TaskProcessorHolder();

  TaskProcessor& operator*() { return *task_processor_; }
  TaskProcessor* operator->() { return &*task_processor_; }

 private:
  utils::UniqueRef<TaskProcessor> task_processor_;
};

void RunOnTaskProcessorSync(TaskProcessor& tp, std::function<void()> user_cb);

}  // namespace engine::impl

USERVER_NAMESPACE_END
