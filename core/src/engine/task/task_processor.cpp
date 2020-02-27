#include "task_processor.hpp"

#include <mutex>

#include <logging/log.hpp>
#include <utils/assert.hpp>
#include <utils/thread_name.hpp>

#include "task_context.hpp"

namespace engine {

TaskProcessor::TaskProcessor(TaskProcessorConfig config,
                             std::shared_ptr<impl::TaskProcessorPools> pools)
    : config_(std::move(config)),
      pools_(std::move(pools)),
      is_shutting_down_(false),
      max_task_queue_wait_time_(std::chrono::microseconds(0)),
      max_task_queue_wait_length_(0),
      task_trace_logger_{nullptr} {
  LOG_TRACE() << "creating task_processor " << Name() << " "
              << "worker_threads=" << config_.worker_threads
              << " thread_name=" << config_.thread_name;
  workers_.reserve(config_.worker_threads);
  for (auto i = config_.worker_threads; i > 0; --i) {
    workers_.emplace_back([this] { ProcessTasks(); });
    utils::SetThreadName(workers_.back(), config_.thread_name);
  }
}

TaskProcessor::~TaskProcessor() {
  InitiateShutdown();

  // Some tasks may be bound but not scheduled yet
  task_counter_.WaitForExhaustion(std::chrono::milliseconds(10));

  task_queue_.enqueue(nullptr);

  for (auto& w : workers_) {
    w.join();
  }

  UASSERT(task_counter_.GetCurrentValue() == 0);
}

void TaskProcessor::InitiateShutdown() {
  is_shutting_down_ = true;

  {
    std::lock_guard<std::mutex> lock(detached_contexts_mutex_);
    for (auto& context : detached_contexts_) {
      context->RequestCancel(Task::CancellationReason::kShutdown);
    }
  }
}

void TaskProcessor::Schedule(impl::TaskContext* context) {
  UASSERT(context);
  if (max_task_queue_wait_length_ && !context->IsCritical()) {
    size_t queue_size = GetTaskQueueSize();
    if (queue_size >= max_task_queue_wait_length_) {
      LOG_WARNING() << "failed to enqueue task: task_queue_ size=" << queue_size
                    << " >= "
                    << "task_queue_size_threshold="
                    << max_task_queue_wait_length_
                    << " task_processor=" << Name();
      HandleOverload(*context);
    }
  }
  if (is_shutting_down_)
    context->RequestCancel(Task::CancellationReason::kShutdown);

  SetTaskQueueWaitTimepoint(context);

  // having native support for intrusive ptrs in lockfree would've been great
  // but oh well
  intrusive_ptr_add_ref(context);

  // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
  task_queue_.enqueue(context);
  // NOTE: task may be executed at this point
}

void TaskProcessor::Adopt(
    boost::intrusive_ptr<impl::TaskContext>&& context_ptr) {
  UASSERT(context_ptr);
  std::unique_lock<std::mutex> lock(detached_contexts_mutex_);
  // SetDetached should be called under lock to synchronize with ProcessTasks
  // IsFinished() cannot change value after the last IsDetached() check
  context_ptr->SetDetached();
  // fast path to avoid hashtable operations
  if (context_ptr->IsFinished()) {
    lock.unlock();
    context_ptr.reset();
    return;
  }
  [[maybe_unused]] auto result =
      detached_contexts_.insert(std::move(context_ptr));
  UASSERT(result.second);
}

impl::CountedCoroutinePtr TaskProcessor::GetCoroutine() {
  return {pools_->GetCoroPool().GetCoroutine(), *this};
}

std::chrono::microseconds TaskProcessor::GetProfilerThreshold() const {
  return config_.profiler_threshold;
}

size_t TaskProcessor::GetTaskTraceMaxCswForNewTask() const {
  thread_local size_t count = 0;
  if (count++ == config_.task_trace_every) {
    count = 0;
    return config_.task_trace_max_csw;
  } else {
    return 0;
  }
}

const std::string& TaskProcessor::GetTaskTraceLoggerName() const {
  return config_.task_trace_logger_name;
}

void TaskProcessor::SetTaskTraceLogger(::logging::LoggerPtr logger) {
  UASSERT(!task_trace_logger_set_);
  task_trace_logger_ = std::move(logger);
  task_trace_logger_set_ = true;
}

::logging::LoggerPtr TaskProcessor::GetTraceLogger() const {
  if (!task_trace_logger_set_) return ::logging::DefaultLogger();
  return task_trace_logger_;
}

impl::TaskContext* TaskProcessor::DequeueTask() {
  impl::TaskContext* buf = nullptr;

  /* Current thread handles only a single TaskProcessor, so it's safe to store
   * a token for the task processor in a thread-local variable.
   */
  thread_local moodycamel::ConsumerToken token(task_queue_);

  task_queue_.wait_dequeue(token, buf);
  GetTaskCounter().AccountTaskSwitchSlow();

  if (!buf) {
    // return "stop" token back
    task_queue_.enqueue(nullptr);
  }

  return buf;
}

void TaskProcessor::ProcessTasks() noexcept {
  while (true) {
    // wrapping instance referenced in EnqueueTask
    boost::intrusive_ptr<impl::TaskContext> context(DequeueTask(),
                                                    /* add_ref =*/false);
    if (!context) break;

    CheckWaitTime(*context);

    bool has_failed = false;
    try {
      context->DoStep();
    } catch (const std::exception& ex) {
      LOG_ERROR() << "uncaught exception from DoStep: " << ex;
      has_failed = true;
    }
    // has_failed is not observable from Adopt()
    // and breaks IsDetached-IsFinished latch
    if (has_failed || (context->IsDetached() && context->IsFinished())) {
      // node with TaskContext is destructed outside of the critical section:
      std::unique_lock<std::mutex> lock(detached_contexts_mutex_);
      [[maybe_unused]] auto node = detached_contexts_.extract(context);
      lock.unlock();
    }
  }
}

void TaskProcessor::SetTaskQueueWaitTimepoint(impl::TaskContext* context) {
  static constexpr size_t kTaskTimestampFrequency = 16;
  thread_local size_t task_count = 0;
  if (task_count++ == kTaskTimestampFrequency) {
    task_count = 0;
    context->SetQueueWaitTimepoint(std::chrono::steady_clock::now());
  } else {
    /* Don't call clock_gettime() too often.
     * This leads to killing some innocent tasks on overload, up to
     * +(kTaskTimestampFrequency-1), we may sacrifice them.
     */
    context->SetQueueWaitTimepoint(std::chrono::steady_clock::time_point());
  }
}

void TaskProcessor::CheckWaitTime(impl::TaskContext& context) {
  const auto max_wait_time = max_task_queue_wait_time_.load();
  if (max_wait_time.count() == 0) {
    task_queue_wait_time_overloaded_ = false;
    return;
  }

  const auto wait_timepoint = context.GetQueueWaitTimepoint();
  if (wait_timepoint != std::chrono::steady_clock::time_point()) {
    const auto wait_time = std::chrono::steady_clock::now() - wait_timepoint;
    const auto wait_time_us =
        std::chrono::duration_cast<std::chrono::microseconds>(wait_time);
    LOG_TRACE() << "queue wait time = " << wait_time_us.count() << "us";

    task_queue_wait_time_overloaded_ = wait_time >= max_wait_time;
  } else {
    // no info, let's pretend this task has the same queue wait time as the
    // previous one
  }

  /* Don't cancel critical tasks, but use their timestamp to cancel other tasks
   */
  if (task_queue_wait_time_overloaded_) {
    HandleOverload(context);
  }
}

void TaskProcessor::HandleOverload(impl::TaskContext& context) {
  GetTaskCounter().AccountTaskOverload();

  if (overload_action_ == TaskProcessorSettings::OverloadAction::kCancel) {
    if (!context.IsCritical()) {
      LOG_WARNING() << "Task with task_id=" << context.GetTaskId()
                    << " was waiting in queue for too long, cancelling.";

      context.RequestCancel(Task::CancellationReason::kOverload);
      GetTaskCounter().AccountTaskCancelOverload();
    } else {
      LOG_TRACE() << "Task with task_id=" << context.GetTaskId()
                  << " was waiting in queue for too long, but it is marked "
                     "as critical, not cancelling.";
    }
  }
}

}  // namespace engine
