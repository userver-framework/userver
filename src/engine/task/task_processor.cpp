#include "task_processor.hpp"

#include <cassert>
#include <mutex>

#include <boost/core/ignore_unused.hpp>
#include <boost/sync/support/std_chrono.hpp>

#include <logging/log.hpp>
#include <utils/thread_name.hpp>

#include "task_context.hpp"

namespace engine {

TaskProcessor::TaskProcessor(TaskProcessorConfig config,
                             std::shared_ptr<impl::TaskProcessorPools> pools)
    : config_(std::move(config)),
      pools_(std::move(pools)),
      is_running_(true),
      is_shutting_down_(false),
      task_queue_size_(0) {
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
  is_shutting_down_ = true;

  {
    std::lock_guard<std::mutex> lock(detached_contexts_mutex_);
    for (auto& context : detached_contexts_) {
      context->RequestCancel(Task::CancellationReason::kShutdown);
    }
  }

  // Some tasks may be bound but not scheduled yet
  task_counter_.WaitForExhaustion(std::chrono::milliseconds(10));

  is_running_ = false;

  for (auto& w : workers_) {
    w.join();
  }

  assert(task_counter_.GetCurrentValue() == 0);
}

void TaskProcessor::Schedule(impl::TaskContext* context) {
  assert(context);
  if (!context->IsCritical() &&
      task_queue_size_ >= config_.task_queue_size_threshold) {
    LOG_WARNING() << "failed to enqueue task: task_queue_size_="
                  << task_queue_size_ << " >= "
                  << "task_queue_size_threshold="
                  << config_.task_queue_size_threshold;
    context->RequestCancel(Task::CancellationReason::kOverload);
  }
  if (is_shutting_down_)
    context->RequestCancel(Task::CancellationReason::kShutdown);

  // having native support for intrusive ptrs in lockfree would've been great
  // but oh well
  intrusive_ptr_add_ref(context);
  ++task_queue_size_;

  task_queue_.enqueue(context);
  // NOTE: task may be executed at this point
}

void TaskProcessor::Adopt(
    boost::intrusive_ptr<impl::TaskContext>&& context_ptr) {
  assert(context_ptr);
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
  auto result = detached_contexts_.insert(std::move(context_ptr));
  assert(result.second);
  boost::ignore_unused(result);
}

impl::TaskContext* TaskProcessor::DequeueTask() {
  impl::TaskContext* buf = nullptr;

  /* Current thread handles only a single TaskProcessor, so it's safe to store
   * a token for the task processor in a thread-local variable.
   */
  thread_local moodycamel::ConsumerToken token(task_queue_);

  /* 'timeout' is used for periodic polling of is_running_
   * in case of TaskProcessor stop.
   */
  static const auto timeout = std::chrono::milliseconds(50);

  while (!task_queue_.wait_dequeue_timed(token, buf, timeout) && is_running_) {
    GetTaskCounter().AccountTaskSwitchSlow();
  }
  return buf;
}

void TaskProcessor::ProcessTasks() noexcept {
  while (true) {
    // wrapping instance referenced in EnqueueTask
    boost::intrusive_ptr<impl::TaskContext> context(DequeueTask(),
                                                    /* add_ref =*/false);
    if (!context) break;
    --task_queue_size_;

    bool has_failed = false;
    try {
      context->DoStep();
    } catch (const std::exception& ex) {
      LOG_ERROR() << "uncaught exception from DoStep: " << ex.what();
      has_failed = true;
    }
    // has_failed is not observable from Adopt()
    // and breaks IsDetached-IsFinished latch
    if (has_failed || (context->IsDetached() && context->IsFinished())) {
      std::lock_guard<std::mutex> lock(detached_contexts_mutex_);
      detached_contexts_.erase(context);
    }
  }
}

}  // namespace engine
