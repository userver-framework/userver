#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <concurrent/impl/interference_shield.hpp>
#include <engine/task/counted_coroutine_ptr.hpp>
#include <engine/task/task_counter.hpp>
#include <engine/task/task_processor_config.hpp>
#include <engine/task/task_queue.hpp>
#include <userver/engine/impl/detached_tasks_sync_block.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace impl {
class TaskContext;
class TaskProcessorPools;
}  // namespace impl

namespace ev {
class ThreadPool;
}  // namespace ev

class TaskProcessor final {
 public:
  TaskProcessor(TaskProcessorConfig, std::shared_ptr<impl::TaskProcessorPools>);
  ~TaskProcessor();

  void InitiateShutdown();

  void Schedule(impl::TaskContext*);

  void Adopt(impl::TaskContext& context);

  impl::CountedCoroutinePtr GetCoroutine();

  ev::ThreadPool& EventThreadPool();

  std::shared_ptr<impl::TaskProcessorPools> GetTaskProcessorPools() {
    return pools_;
  }

  const std::string& Name() const { return config_.name; }

  impl::TaskCounter& GetTaskCounter() noexcept { return *task_counter_; }

  const impl::TaskCounter& GetTaskCounter() const { return *task_counter_; }

  size_t GetTaskQueueSize() const { return task_queue_.GetSizeApprox(); }

  size_t GetWorkerCount() const { return workers_.size(); }

  void SetSettings(const TaskProcessorSettings& settings);

  std::chrono::microseconds GetProfilerThreshold() const;

  bool ShouldProfilerForceStacktrace() const;

  size_t GetTaskTraceMaxCswForNewTask() const;

  const std::string& GetTaskTraceLoggerName() const;

  void SetTaskTraceLogger(logging::LoggerPtr logger);

  logging::LoggerPtr GetTaskTraceLogger() const;

 private:
  void Cleanup() noexcept;

  impl::TaskContext* DequeueTask();

  void ProcessTasks() noexcept;

  void CheckWaitTime(impl::TaskContext& context);

  void SetTaskQueueWaitTimeOverloaded(bool new_value);

  void HandleOverload(impl::TaskContext& context);

  const TaskProcessorConfig config_;
  std::atomic<std::chrono::microseconds> task_profiler_threshold_{{}};
  std::atomic<bool> profiler_force_stacktrace_{false};

  std::shared_ptr<impl::TaskProcessorPools> pools_;

  std::atomic<bool> is_shutting_down_{false};

  impl::TaskQueue task_queue_;

  std::atomic<std::chrono::microseconds> sensor_task_queue_wait_time_{};
  std::atomic<std::chrono::microseconds> max_task_queue_wait_time_{{}};
  std::atomic<size_t> max_task_queue_wait_length_{0};
  std::atomic<TaskProcessorSettings::OverloadAction> overload_action_{
      TaskProcessorSettings::OverloadAction::kIgnore};

  std::vector<std::thread> workers_;
  std::atomic<bool> task_trace_logger_set_{false};
  logging::LoggerPtr task_trace_logger_{nullptr};

  concurrent::impl::InterferenceShield<impl::TaskCounter> task_counter_;
  concurrent::impl::InterferenceShield<impl::DetachedTasksSyncBlock>
      detached_contexts_{impl::DetachedTasksSyncBlock::StopMode::kCancel};
  concurrent::impl::InterferenceShield<std::atomic<bool>>
      task_queue_wait_time_overloaded_{false};
};

/// Register a function that runs on all threads on task processor creation.
/// Used for pre-initializing thread_local variables with heavy constructors
/// (constructor that does blocking system calls, file access, ...):
///
/// @note It is a low-level function. You might not want to use it.
void RegisterThreadStartedHook(std::function<void()>);

}  // namespace engine

USERVER_NAMESPACE_END
