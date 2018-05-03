#pragma once

#include <memory>
#include <shared_mutex>
#include <unordered_set>
#include <vector>

#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/stack.hpp>

#include <engine/coro/pool.hpp>
#include <engine/ev/thread_pool.hpp>

#include "task.hpp"
#include "task_processor_config.hpp"
#include "task_worker.hpp"

namespace engine {

class TaskProcessor {
 public:
  using CoroPool = coro::Pool<Task>;

  TaskProcessor(const TaskProcessorConfig& config, CoroPool& coro_pool,
                ev::ThreadPool& event_thread_pool);
  ~TaskProcessor();

  bool AddTask(Task* task, bool can_fail = false);

  Task* NextTask(int worker_id);

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

    virtual ~RefCounter() { --task_processor_.async_task_counter_; }

   private:
    TaskProcessor& task_processor_;
  };

 private:
  bool EnqueueTask(Task* task, bool can_fail = false);
  void TryRunTask();

  const TaskProcessorConfig& config_;

  CoroPool& coro_pool_;
  ev::ThreadPool& event_thread_pool_;

  std::shared_timed_mutex task_queue_mutex_;
  std::atomic<size_t> task_queue_size_;
  boost::lockfree::queue<Task*> task_queue_;
  std::unique_ptr<ev::ThreadPool> worker_thread_pool_;
  std::vector<std::unique_ptr<TaskWorker>> workers_;
  boost::lockfree::stack<int, boost::lockfree::fixed_sized<true>>
      workers_stack_;

  std::atomic<size_t> async_task_counter_;
};

}  // namespace engine
