#pragma once

#include <atomic>
#include <condition_variable>
#include <shared_mutex>
#include <thread>
#include <vector>

#include <boost/lockfree/queue.hpp>

#include <engine/coro/pool.hpp>
#include <engine/ev/thread_pool.hpp>

#include "task.hpp"
#include "task_processor_config.hpp"

namespace engine {

class TaskProcessor {
 public:
  using CoroPool = coro::Pool<Task>;

  TaskProcessor(const TaskProcessorConfig& config, CoroPool& coro_pool,
                ev::ThreadPool& event_thread_pool);
  ~TaskProcessor();

  bool AddTask(Task* task, bool can_fail = false);

  CoroPool& GetCoroPool() { return coro_pool_; }
  ev::ThreadPool& EventThreadPool() { return event_thread_pool_; }
  const std::string& Name() const { return config_.name; }

  class RefCounter {
   public:
    explicit RefCounter(TaskProcessor& task_processor)
        : task_processor_(task_processor) {
      ++task_processor_.async_task_counter_;
    }

    RefCounter(const RefCounter&) = delete;
    RefCounter(RefCounter&&) = delete;
    RefCounter& operator=(const RefCounter&) = delete;
    RefCounter& operator=(RefCounter&&) = delete;

    ~RefCounter() { --task_processor_.async_task_counter_; }

   private:
    TaskProcessor& task_processor_;
  };

 private:
  bool EnqueueTask(Task* task, bool can_fail);

  void ProcessTasks() noexcept;

  const TaskProcessorConfig& config_;

  CoroPool& coro_pool_;
  ev::ThreadPool& event_thread_pool_;

  std::atomic<bool> is_running_;

  // Shared lock is acquired for insertions into task_queue_.
  // Used for double checking/pausing workers on queue exhaustion.
  std::shared_timed_mutex task_queue_mutex_;
  std::condition_variable_any task_available_cv_;
  boost::lockfree::queue<Task*> task_queue_;
  std::atomic<size_t> task_queue_size_;

  std::vector<std::thread> workers_;
  std::atomic<size_t> async_task_counter_;
};

}  // namespace engine
