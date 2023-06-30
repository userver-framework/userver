#include "task_processor.hpp"

#include <sys/types.h>
#include <csignal>

#include <fmt/format.h>

#include <concurrent/impl/latch.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/static_registration.hpp>
#include <userver/utils/rand.hpp>
#include <userver/utils/thread_name.hpp>
#include <userver/utils/threads.hpp>

#include <engine/task/counted_coroutine_ptr.hpp>
#include <engine/task/task_context.hpp>
#include <engine/task/task_processor_pools.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {
namespace {

void SetTaskQueueWaitTimepoint(impl::TaskContext* context) {
  static constexpr size_t kTaskTimestampInterval = 4;
  thread_local size_t task_count = 0;
  if (task_count++ == kTaskTimestampInterval) {
    task_count = 0;
    context->SetQueueWaitTimepoint(std::chrono::steady_clock::now());
  } else {
    /* Don't call clock_gettime() too often.
     * This leads to killing some innocent tasks on overload, up to
     * +(kTaskTimestampInterval-1), we may sacrifice them.
     */
    context->SetQueueWaitTimepoint(std::chrono::steady_clock::time_point());
  }
}

// Hooks are modified only before task processors created and only in main
// thread, so it doesnt' need any synchronization.
std::vector<std::function<void()>>& ThreadStartedHooks() {
  static std::vector<std::function<void()>> thread_started_hooks;
  return thread_started_hooks;
}

void EmitMagicNanosleep() {
  // If we're ptrace'd (e.g. by strace), the magic syscall tells a tracer
  // that all startup stuff of the current thread is done.
  // Before this timepoint we could do blocking syscalls.
  // From now on, every blocking syscall is a bug.
  const struct timespec ts = {0, 42};
  nanosleep(&ts, nullptr);
}

void TaskProcessorThreadStartedHook() {
  utils::impl::AssertStaticRegistrationFinished();
  (void)utils::DefaultRandom();
  for (const auto& func : ThreadStartedHooks()) {
    func();
  }
  EmitMagicNanosleep();
}

}  // namespace

TaskProcessor::TaskProcessor(TaskProcessorConfig config,
                             std::shared_ptr<impl::TaskProcessorPools> pools)
    : task_counter_(config.worker_threads),
      task_queue_(config),
      config_(std::move(config)),
      pools_(std::move(pools)) {
  utils::impl::FinishStaticRegistration();
  try {
    LOG_INFO() << "creating task_processor " << Name() << " "
               << "worker_threads=" << config_.worker_threads
               << " thread_name=" << config_.thread_name;
    concurrent::impl::Latch workers_left{
        static_cast<std::ptrdiff_t>(config_.worker_threads)};
    workers_.reserve(config_.worker_threads);
    for (size_t i = 0; i < config_.worker_threads; ++i) {
      workers_.emplace_back([this, i, &workers_left] {
        PrepareWorkerThread(i);
        workers_left.count_down();
        ProcessTasks();
      });
    }
    workers_left.wait();
  } catch (...) {
    Cleanup();
    throw;
  }
}

TaskProcessor::~TaskProcessor() { Cleanup(); }

void TaskProcessor::Cleanup() noexcept {
  InitiateShutdown();

  // Some tasks may be bound but not scheduled yet
  task_counter_.WaitForExhaustion();

  task_queue_.StopProcessing();

  for (auto& w : workers_) {
    w.join();
  }

  UASSERT(!task_counter_.MayHaveTasksAlive());
}

void TaskProcessor::InitiateShutdown() {
  is_shutting_down_ = true;
  detached_contexts_->RequestCancellation(TaskCancellationReason::kShutdown);
}

void TaskProcessor::Schedule(impl::TaskContext* context) {
  UASSERT(context);
  if (max_task_queue_wait_length_ && !context->IsCritical()) {
    const auto queue_size = GetTaskQueueSize();
    if (queue_size >= max_task_queue_wait_length_) {
      LOG_LIMITED_WARNING()
          << "failed to enqueue task: task_queue_ size=" << queue_size << " >= "
          << "task_queue_size_threshold=" << max_task_queue_wait_length_
          << " task_processor=" << Name();
      HandleOverload(*context);
    }
  }
  if (is_shutting_down_)
    context->RequestCancel(TaskCancellationReason::kShutdown);

  SetTaskQueueWaitTimepoint(context);

  task_queue_.Push(context);
}

void TaskProcessor::Adopt(impl::TaskContext& context) {
  detached_contexts_->Add(context);
}

ev::ThreadPool& TaskProcessor::EventThreadPool() {
  return pools_->EventThreadPool();
}

impl::CountedCoroutinePtr TaskProcessor::GetCoroutine() {
  return {pools_->GetCoroPool().GetCoroutine(), *this};
}

void TaskProcessor::SetSettings(const TaskProcessorSettings& settings) {
  sensor_task_queue_wait_time_ = settings.sensor_wait_queue_time_limit;
  max_task_queue_wait_time_ = settings.wait_queue_time_limit;
  max_task_queue_wait_length_ = settings.wait_queue_length_limit;
  overload_action_ = settings.overload_action;

  auto threshold = settings.profiler_execution_slice_threshold;
  if (threshold.count() > 0) {
    auto old_threshold = task_profiler_threshold_.exchange(threshold);
    if (old_threshold.count() == 0) {
      LOG_WARNING() << fmt::format(
          "Task profiling is now enabled for task processor '{}' "
          "(threshold={}us), you may "
          "change settings or disable it in "
          "USERVER_TASK_PROCESSOR_PROFILER_DEBUG config",
          config_.thread_name, threshold.count());
    }
  } else {
    auto old_threshold =
        task_profiler_threshold_.exchange(std::chrono::microseconds(0));
    if (old_threshold.count() > 0) {
      LOG_WARNING() << fmt::format(
          "Task profiling is now disabled for task processor '{}', you may "
          "enable it in USERVER_TASK_PROCESSOR_PROFILER_DEBUG config",
          config_.thread_name);
    }
  }
  profiler_force_stacktrace_.store(settings.profiler_force_stacktrace);
}

std::chrono::microseconds TaskProcessor::GetProfilerThreshold() const {
  return task_profiler_threshold_.load();
}

bool TaskProcessor::ShouldProfilerForceStacktrace() const {
  return profiler_force_stacktrace_.load();
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

void TaskProcessor::SetTaskTraceLogger(logging::LoggerPtr logger) {
  task_trace_logger_ = std::move(logger);
  [[maybe_unused]] const auto was_task_trace_logger_set =
      task_trace_logger_set_.exchange(true, std::memory_order_release);
  UASSERT(!was_task_trace_logger_set);
}

logging::LoggerPtr TaskProcessor::GetTaskTraceLogger() const {
  // logger macros should be ready to deal with null logger
  if (!task_trace_logger_set_.load(std::memory_order_acquire)) return {};
  return task_trace_logger_;
}

void RegisterThreadStartedHook(std::function<void()> func) {
  utils::impl::AssertStaticRegistrationAllowed(
      "Calling engine::RegisterThreadStartedHook()");
  ThreadStartedHooks().push_back(std::move(func));
}

void TaskProcessor::PrepareWorkerThread(std::size_t index) noexcept {
  switch (config_.os_scheduling) {
    case OsScheduling::kNormal:
      break;
    case OsScheduling::kLowPriority:
      utils::SetCurrentThreadLowPriorityScheduling();
      break;
    case OsScheduling::kIdle:
      utils::SetCurrentThreadIdleScheduling();
      break;
  }

  utils::SetCurrentThreadName(fmt::format("{}_{}", config_.thread_name, index));

  impl::SetLocalTaskCounterData(task_counter_, index);

  TaskProcessorThreadStartedHook();
}

void TaskProcessor::ProcessTasks() noexcept {
  while (true) {
    auto context = task_queue_.PopBlocking();
    if (!context) break;

    GetTaskCounter().AccountTaskSwitchSlow();
    CheckWaitTime(*context);

    bool has_failed = false;
    try {
      context->DoStep();
    } catch (const std::exception& ex) {
      LOG_ERROR() << "uncaught exception from DoStep: " << ex;
      has_failed = true;
    }

    if (has_failed || context->IsFinished()) {
      context->FinishDetached();
    }
  }
}

void TaskProcessor::CheckWaitTime(impl::TaskContext& context) {
  const auto max_wait_time = max_task_queue_wait_time_.load();
  const auto sensor_wait_time = sensor_task_queue_wait_time_.load();

  if (max_wait_time.count() == 0 && sensor_wait_time.count() == 0) {
    SetTaskQueueWaitTimeOverloaded(false);
    return;
  }

  const auto wait_timepoint = context.GetQueueWaitTimepoint();
  if (wait_timepoint != std::chrono::steady_clock::time_point()) {
    const auto wait_time = std::chrono::steady_clock::now() - wait_timepoint;
    const auto wait_time_us =
        std::chrono::duration_cast<std::chrono::microseconds>(wait_time);
    LOG_TRACE() << "queue wait time = " << wait_time_us.count() << "us";

    SetTaskQueueWaitTimeOverloaded(max_wait_time.count() &&
                                   wait_time >= max_wait_time);

    if (sensor_wait_time.count() && wait_time >= sensor_wait_time) {
      GetTaskCounter().AccountTaskOverloadSensor();
    } else {
      GetTaskCounter().AccountTaskNoOverloadSensor();
    }
  } else {
    // no info, let's pretend this task has the same queue wait time as the
    // previous one
  }

  // Don't cancel critical tasks, but use their timestamp to cancel other tasks
  if (task_queue_wait_time_overloaded_->load()) {
    HandleOverload(context);
  }
}

void TaskProcessor::SetTaskQueueWaitTimeOverloaded(bool new_value) noexcept {
  auto& atomic = *task_queue_wait_time_overloaded_;
  // The check helps to reduce contention.
  if (atomic.load(std::memory_order_relaxed) != new_value) {
    atomic.store(new_value, std::memory_order_relaxed);
  }
}

void TaskProcessor::HandleOverload(impl::TaskContext& context) {
  GetTaskCounter().AccountTaskOverload();

  if (overload_action_ == TaskProcessorSettings::OverloadAction::kCancel) {
    if (!context.IsCritical()) {
      LOG_LIMITED_WARNING()
          << "Task with task_id=" << logging::HexShort(context.GetTaskId())
          << " was waiting in queue for too long, cancelling.";

      context.RequestCancel(TaskCancellationReason::kOverload);
      GetTaskCounter().AccountTaskCancelOverload();
    } else {
      LOG_TRACE() << "Task with task_id="
                  << logging::HexShort(context.GetTaskId())
                  << " was waiting in queue for too long, but it is marked "
                     "as critical, not cancelling.";
    }
  }
}

}  // namespace engine

USERVER_NAMESPACE_END
