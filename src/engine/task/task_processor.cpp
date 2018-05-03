#include "task_processor.hpp"

#include <cassert>
#include <mutex>

#include <logging/log.hpp>

namespace engine {
namespace {

const size_t kInitTaskQueueCapacity = 64;

}  // namespace

TaskProcessor::TaskProcessor(const TaskProcessorConfig& config,
                             CoroPool& coro_pool,
                             ev::ThreadPool& event_thread_pool)
    : config_(config),
      coro_pool_(coro_pool),
      event_thread_pool_(event_thread_pool),
      task_queue_size_(0),
      task_queue_(kInitTaskQueueCapacity),
      workers_stack_(config.worker_threads),
      async_task_counter_(0) {
  LOG_TRACE() << "creating task_processor " << Name() << " "
              << "worker_threads=" << config.worker_threads
              << " thread_name=" << config.thread_name;
  worker_thread_pool_ = std::make_unique<ev::ThreadPool>(config.worker_threads,
                                                         config.thread_name);

  auto worker_threads = worker_thread_pool_->NextThreads(config.worker_threads);
  workers_.reserve(config.worker_threads);
  for (decltype(config.worker_threads) i = 0; i < config.worker_threads; ++i) {
    workers_.emplace_back(
        std::make_unique<TaskWorker>(*worker_threads[i], i, *this));
    workers_stack_.push(i);
  }
}

TaskProcessor::~TaskProcessor() {
  while (async_task_counter_ > 0)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

bool TaskProcessor::AddTask(Task* task, bool can_fail) {
  if (!EnqueueTask(task, can_fail)) return false;

  TryRunTask();

  return true;
}

Task* TaskProcessor::NextTask(int worker_id) {
  Task* task;
  if (!task_queue_.pop(task)) {
    std::unique_lock<std::shared_timed_mutex> lock(task_queue_mutex_);
    // prevents AddTask() execution between task_queue_.pop() and
    // workers_stack_.push()
    if (!task_queue_.pop(task)) {
      workers_stack_.push(worker_id);
      return nullptr;
    }
  }
  --task_queue_size_;
  task->SetState(Task::State::kRunning);
  return task;
}

bool TaskProcessor::EnqueueTask(Task* task, bool can_fail) {
  assert(task);
  if (can_fail && task_queue_size_ >= config_.task_queue_size_threshold) {
    LOG_WARNING() << "failed to enqueue task: task_queue_size_="
                  << task_queue_size_ << " >= "
                  << "task_queue_size_threshold="
                  << config_.task_queue_size_threshold;
    task->Fail();
    return false;
  }

  task->SetState(Task::State::kQueued);
  {
    ++task_queue_size_;
    std::shared_lock<std::shared_timed_mutex> lock(task_queue_mutex_);
    task_queue_.push(task);
  }

  return true;
}

void TaskProcessor::TryRunTask() {
  int worker_id;
  if (!workers_stack_.pop(worker_id)) return;
  Task* task = NextTask(worker_id);
  if (task) workers_[worker_id]->RunTaskAsync(task);
}

}  // namespace engine
