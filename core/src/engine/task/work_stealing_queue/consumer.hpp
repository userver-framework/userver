#pragma once
#include <moodycamel/blockingconcurrentqueue.h>
#include <moodycamel/lightweightsemaphore.h>
#include <array>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <cstddef>
#include <random>
#include "engine/task/task_context.hpp"
#include "local_queue.hpp"

#include <engine/task/task_processor_config.hpp>

constexpr std::size_t kLocalQueueMaxSize = 63;
constexpr std::size_t kConsumerStealBufferSize = 32;
constexpr std::size_t kStealAttempts = 4;

USERVER_NAMESPACE_BEGIN
namespace engine {

class WorkStealingTaskQueue;
class ConsumersManager;

class Consumer final {
 public:
  Consumer();

  Consumer(WorkStealingTaskQueue* owner, ConsumersManager* consumers_manager,
           std::size_t inner_index);

  Consumer(const Consumer& rhs);

  bool Push(impl::TaskContext* ctx);

  impl::TaskContext* Pop();

  std::size_t LocalQueueSize() const;

  WorkStealingTaskQueue* GetOwner() const;

 private:
  friend ConsumersManager;
  bool Stopped();
  void MoveTasksToGlobalQueue(impl::TaskContext* extra);
  impl::TaskContext* StealFromAnotherConsumer(std::size_t attempts,
                                              std::size_t to_steal);
  std::size_t Steal(impl::TaskContext** buffer, std::size_t size);
  impl::TaskContext* PopFromGlobalQueue();
  impl::TaskContext* TryPop();
  impl::TaskContext* DoPop();
  void Sleep(int val);
  void WakeUp();

  userver::engine::LocalQueue<impl::TaskContext, kLocalQueueMaxSize> local_queue_;
  WorkStealingTaskQueue* const owner_;
  ConsumersManager* const consumers_manager_;
  const std::size_t inner_index_;
  const std::size_t default_steal_size_;
  std::array<impl::TaskContext*, kConsumerStealBufferSize> steal_buffer_;
  std::mt19937 rnd_;
  std::size_t steps_count_;
  std::size_t pushed_;
  std::atomic<int> sleep_counter_;
};

}  // namespace engine
USERVER_NAMESPACE_END

#include "consumers_manager.hpp"
