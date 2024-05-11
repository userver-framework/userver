#pragma once

#include <moodycamel/blockingconcurrentqueue.h>
#include <moodycamel/lightweightsemaphore.h>
#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <engine/task/task_processor_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace impl {
class TaskContext;
}  // namespace impl

class TaskQueue final {
 public:
  explicit TaskQueue(const TaskProcessorConfig& config);

  void Push(boost::intrusive_ptr<impl::TaskContext>&& context);

  // Returns nullptr as a stop signal
  boost::intrusive_ptr<impl::TaskContext> PopBlocking();

  void StopProcessing();

  // This method is much slower,
  // but more accurate than GetSizeApproximate()
  std::size_t GetSize() const noexcept;

  // This method is much faster,
  // but less accurate than GetSize()
  std::size_t GetSizeApproximate() const noexcept;

 private:
  void DoPush(impl::TaskContext* context);

  impl::TaskContext* DoPopBlocking(moodycamel::ConsumerToken& token);

  void UpdateQueueSize();

  moodycamel::ConcurrentQueue<impl::TaskContext*> queue_;
  moodycamel::LightweightSemaphore queue_semaphore_;
  std::atomic<std::size_t> queue_size_cached_{0};
};

}  // namespace engine

USERVER_NAMESPACE_END
