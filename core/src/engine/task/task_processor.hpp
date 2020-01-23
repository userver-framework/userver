#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <unordered_set>
#include <vector>

#include <moodycamel/blockingconcurrentqueue.h>
#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <engine/task/counted_coroutine_ptr.hpp>
#include <engine/task/task_counter.hpp>
#include <engine/task/task_processor_config.hpp>
#include <engine/task/task_processor_pools.hpp>

namespace engine {
namespace impl {

class TaskContext;

struct TaskContextPtrHash {
  auto operator()(const boost::intrusive_ptr<TaskContext>& ptr) const {
    return std::hash<TaskContext*>()(ptr.get());
  }
};

}  // namespace impl

struct TaskProcessorSettings {
  size_t wait_queue_length_limit{0};
  std::chrono::microseconds wait_queue_time_limit{0};

  enum class OverloadAction { kCancel, kIgnore };
  OverloadAction overload_action{OverloadAction::kIgnore};
};

class TaskProcessor final {
 public:
  TaskProcessor(TaskProcessorConfig, std::shared_ptr<impl::TaskProcessorPools>);
  ~TaskProcessor();

  void Schedule(impl::TaskContext*);
  void Adopt(boost::intrusive_ptr<impl::TaskContext>&&);

  impl::CountedCoroutinePtr GetCoroutine();

  ev::ThreadPool& EventThreadPool() { return pools_->EventThreadPool(); }
  std::shared_ptr<impl::TaskProcessorPools> GetTaskProcessorPools() {
    return pools_;
  }
  const std::string& Name() const { return config_.name; }

  impl::TaskCounter& GetTaskCounter() { return task_counter_; }

  const impl::TaskCounter& GetTaskCounter() const { return task_counter_; }

  size_t GetTaskQueueSize() const { return task_queue_.size_approx(); }

  size_t GetWorkerCount() const { return workers_.size(); }

  void SetSettings(const TaskProcessorSettings& settings) {
    max_task_queue_wait_time_ = settings.wait_queue_time_limit;
    max_task_queue_wait_length_ = settings.wait_queue_length_limit;
    overload_action_ = settings.overload_action;
  }

  std::chrono::microseconds GetProfilerThreshold() const;

  size_t GetTaskTraceMaxCswForNewTask() const;

  const std::string& GetTaskTraceLoggerName() const;

  void SetTaskTraceLogger(::logging::LoggerPtr logger);

  ::logging::LoggerPtr GetTraceLogger() const;

 private:
  impl::TaskContext* DequeueTask();

  void ProcessTasks() noexcept;

  void SetTaskQueueWaitTimepoint(impl::TaskContext* context);

  void CheckWaitTime(impl::TaskContext& context);

  void HandleOverload(impl::TaskContext& context);

  const TaskProcessorConfig config_;
  std::shared_ptr<impl::TaskProcessorPools> pools_;

  std::atomic<bool> is_shutting_down_;

  std::mutex detached_contexts_mutex_;
  std::unordered_set<boost::intrusive_ptr<impl::TaskContext>,
                     impl::TaskContextPtrHash>
      detached_contexts_;

  moodycamel::BlockingConcurrentQueue<impl::TaskContext*> task_queue_;

  std::atomic<std::chrono::microseconds> max_task_queue_wait_time_{};
  std::atomic<size_t> max_task_queue_wait_length_{0};
  TaskProcessorSettings::OverloadAction overload_action_{
      TaskProcessorSettings::OverloadAction::kIgnore};
  bool task_queue_wait_time_overloaded_{false};

  std::vector<std::thread> workers_;
  impl::TaskCounter task_counter_;
  std::atomic<bool> task_trace_logger_set_{false};
  ::logging::LoggerPtr task_trace_logger_{nullptr};
};

}  // namespace engine
