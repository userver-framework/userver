#include "task_processor.hpp"

#include <cassert>
#include <mutex>

#include <logging/log.hpp>
#include <utils/thread_name.hpp>

#include "task.hpp"

namespace engine {
namespace {

const size_t kInitTaskQueueCapacity = 64;

class CurrentTaskHolder {
 public:
  explicit CurrentTaskHolder(Task* task) { current_task::SetCurrentTask(task); }
  ~CurrentTaskHolder() { current_task::SetCurrentTask(nullptr); }
};

}  // namespace

TaskProcessor::TaskProcessor(const TaskProcessorConfig& config,
                             CoroPool& coro_pool,
                             ev::ThreadPool& event_thread_pool)
    : config_(config),
      coro_pool_(coro_pool),
      event_thread_pool_(event_thread_pool),
      is_running_(true),
      task_queue_(kInitTaskQueueCapacity),
      task_queue_size_(0),
      async_task_counter_(0) {
  LOG_TRACE() << "creating task_processor " << Name() << " "
              << "worker_threads=" << config.worker_threads
              << " thread_name=" << config.thread_name;
  workers_.reserve(config.worker_threads);
  for (auto i = config.worker_threads; i > 0; --i) {
    workers_.emplace_back([this] { ProcessTasks(); });
    utils::SetThreadName(workers_.back(), config.thread_name);
  }
}

TaskProcessor::~TaskProcessor() {
  while (async_task_counter_ > 0)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

  {
    std::shared_lock<std::shared_timed_mutex> lock(task_queue_mutex_);
    is_running_ = false;
    task_available_cv_.notify_all();
  }
  for (auto& w : workers_) {
    w.join();
  }
}

bool TaskProcessor::AddTask(Task* task, bool can_fail) {
  if (!EnqueueTask(task, can_fail)) return false;
  task_available_cv_.notify_one();
  return true;
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
  ++task_queue_size_;
  {
    std::shared_lock<std::shared_timed_mutex> lock(task_queue_mutex_);
    task_queue_.push(task);
  }

  return true;
}

void TaskProcessor::ProcessTasks() noexcept {
  while (true) {
    Task* task = nullptr;
    if (!task_queue_.pop(task)) {
      std::unique_lock<std::shared_timed_mutex> lock(task_queue_mutex_);
      task_available_cv_.wait(lock, [this, &task] {
        return task_queue_.pop(task) || !is_running_;
      });
      if (!is_running_) break;
    }
    --task_queue_size_;

    auto task_state = Task::State::kRunning;
    task->SetState(task_state);
    {
      CurrentTaskHolder current_task(task);
      task_state = task->RunTask();
    }
    if (task_state == Task::State::kWaiting) task->Sleep();
  }
}

}  // namespace engine
