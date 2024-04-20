#pragma once

#include <array>
#include <condition_variable>
#include <cstddef>
#include <random>

#include <moodycamel/blockingconcurrentqueue.h>
#include <moodycamel/lightweightsemaphore.h>
#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <engine/task/task_context.hpp>
#include <engine/task/work_stealing_queue/local_queue.hpp>

USERVER_NAMESPACE_BEGIN
namespace engine {

class WorkStealingTaskQueue;
class ConsumersManager;

class Consumer final {
 public:
  Consumer(WorkStealingTaskQueue& owner, ConsumersManager& consumers_manager);

  void Push(impl::TaskContext* ctx);

  impl::TaskContext* PopBlocking();

  std::size_t GetLocalQueueSize() const noexcept;

  WorkStealingTaskQueue* GetOwner() const noexcept;

 private:
  friend ConsumersManager;
  friend WorkStealingTaskQueue;
  void SetIndex(std::size_t index) noexcept;
  bool IsStopped() const noexcept;
  void MoveTasksToGlobalQueue(impl::TaskContext* extra);
  impl::TaskContext* StealFromAnotherConsumer(const std::size_t attempts,
                                              std::size_t to_steal);
  std::size_t Steal(utils::span<impl::TaskContext*> buffer);
  impl::TaskContext* TryPopFromOwnerQueue(bool is_global);
  impl::TaskContext* ProbabilisticPopFromOwnerQueues();
  impl::TaskContext* TryPop();
  impl::TaskContext* TryPopBeforeSleep();
  impl::TaskContext* DoPop();
  void Sleep(const std::int32_t old_sleep_counter);
  void WakeUp();

  LocalQueue<impl::TaskContext, 63> local_queue_{};
  WorkStealingTaskQueue& owner_;
  ConsumersManager& consumers_manager_;
  std::size_t inner_index_{0};
  std::array<impl::TaskContext*, 32> steal_buffer_{};
  std::minstd_rand rnd_;
  std::size_t steps_count_;
  std::atomic<std::int32_t> sleep_counter_{0};
#ifndef __linux__
  std::condition_variable cv_;
  std::mutex mutex_;
#endif
};

}  // namespace engine
USERVER_NAMESPACE_END

#include "consumers_manager.hpp"
