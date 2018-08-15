#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <unordered_set>
#include <vector>

#include <boost/lockfree/queue.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <engine/coro/pool.hpp>
#include <engine/ev/thread_pool.hpp>

#include "task_processor_config.hpp"

namespace engine {
namespace impl {

class TaskContext;

struct TaskContextPtrHash {
  auto operator()(const boost::intrusive_ptr<TaskContext>& ptr) const {
    return std::hash<TaskContext*>()(ptr.get());
  }
};

}  // namespace impl

class TaskProcessor {
 public:
  using CoroPool = coro::Pool<impl::TaskContext>;

  class CountedRef {
   public:
    explicit CountedRef(TaskProcessor& task_processor)
        : task_processor_(task_processor) {
      ++task_processor_.async_task_counter_;
    }
    ~CountedRef() { --task_processor_.async_task_counter_; }

    CountedRef(const CountedRef&) = delete;
    CountedRef(CountedRef&&) = delete;
    CountedRef& operator=(const CountedRef&) = delete;
    CountedRef& operator=(CountedRef&&) = delete;

   private:
    TaskProcessor& task_processor_;
  };

  TaskProcessor(const TaskProcessorConfig& config, CoroPool& coro_pool,
                ev::ThreadPool& event_thread_pool);
  ~TaskProcessor();

  void Schedule(impl::TaskContext*);
  void Adopt(boost::intrusive_ptr<impl::TaskContext>&&);

  CoroPool& GetCoroPool() { return coro_pool_; }
  ev::ThreadPool& EventThreadPool() { return event_thread_pool_; }
  const std::string& Name() const { return config_.name; }

 private:
  void ProcessTasks() noexcept;

  const TaskProcessorConfig& config_;

  CoroPool& coro_pool_;
  ev::ThreadPool& event_thread_pool_;

  std::atomic<bool> is_running_;
  std::atomic<bool> is_shutting_down_;

  std::mutex detached_contexts_mutex_;
  std::unordered_set<boost::intrusive_ptr<impl::TaskContext>,
                     impl::TaskContextPtrHash>
      detached_contexts_;

  // Shared lock is acquired for insertions into task_queue_.
  // Used for double checking/pausing workers on queue exhaustion.
  std::shared_timed_mutex task_queue_mutex_;
  std::condition_variable_any task_available_cv_;
  boost::lockfree::queue<impl::TaskContext*> task_queue_;
  std::atomic<size_t> task_queue_size_;

  std::vector<std::thread> workers_;
  std::atomic<size_t> async_task_counter_;
};

}  // namespace engine
