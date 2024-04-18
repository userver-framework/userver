#pragma once
#include <moodycamel/blockingconcurrentqueue.h>
#include <moodycamel/lightweightsemaphore.h>
#include <array>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <condition_variable>
#include <cstddef>
#include <engine/task/task_context.hpp>
#include <engine/task/work_stealing_queue/local_queue.hpp>
#include <random>

USERVER_NAMESPACE_BEGIN
namespace engine {

class WorkStealingTaskQueue;
class ConsumersManager;

class Consumer final {
 public:
  Consumer(WorkStealingTaskQueue* owner, ConsumersManager* consumers_manager);

  bool Push(impl::TaskContext* ctx);

  impl::TaskContext* Pop();

  std::size_t LocalQueueSize() const noexcept;

  WorkStealingTaskQueue* GetOwner() const noexcept;

 private:
  friend ConsumersManager;
  friend WorkStealingTaskQueue;
  void SetIndex(std::size_t index) noexcept;
  bool IsStopped() const noexcept;
  void MoveTasksToGlobalQueue(impl::TaskContext* extra);
  impl::TaskContext* StealFromAnotherConsumer(std::size_t attempts,
                                              std::size_t to_steal);
  std::size_t Steal(utils::span<impl::TaskContext*> buffer);
  impl::TaskContext* TryPopFromGlobalQueue();
  impl::TaskContext* TryPop();
  impl::TaskContext* DoPop();
  void Sleep(std::int32_t val);
  void WakeUp();

  LocalQueue<impl::TaskContext, 63> local_queue_;
  WorkStealingTaskQueue* const owner_;
  ConsumersManager* const consumers_manager_;
  std::size_t inner_index_{0};
  std::array<impl::TaskContext*, 32> steal_buffer_;
  std::mt19937 rnd_;
  std::size_t steps_count_;
  std::size_t pushed_;
  std::atomic<std::int32_t> sleep_counter_;
  std::condition_variable cv_;
  std::mutex mutex_;
  // #ifndef __linux__

  // #endif
};

}  // namespace engine
USERVER_NAMESPACE_END

#include "consumers_manager.hpp"
