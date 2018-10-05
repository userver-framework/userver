#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <unordered_set>
#include <vector>

#include <boost/lockfree/queue.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>

#include "task_counter.hpp"
#include "task_processor_config.hpp"
#include "task_processor_pools.hpp"

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
  using CoroPool = impl::TaskProcessorPools::CoroPool;

  TaskProcessor(TaskProcessorConfig, std::shared_ptr<impl::TaskProcessorPools>);
  ~TaskProcessor();

  void Schedule(impl::TaskContext*);
  void Adopt(boost::intrusive_ptr<impl::TaskContext>&&);

  CoroPool& GetCoroPool() { return pools_->GetCoroPool(); }
  ev::ThreadPool& EventThreadPool() { return pools_->EventThreadPool(); }
  const std::string& Name() const { return config_.name; }

  impl::TaskCounter& GetTaskCounter() { return task_counter_; }

 private:
  void ProcessTasks() noexcept;

  const TaskProcessorConfig config_;
  std::shared_ptr<impl::TaskProcessorPools> pools_;

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
  impl::TaskCounter task_counter_;
};

}  // namespace engine
