#include <engine/task/task_queue.hpp>

#include <atomic>

#include <userver/utils/rand.hpp>

#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace {
constexpr std::size_t kSemaphoreInitialCount = 0;
}

TaskQueue::TaskQueue(const TaskProcessorConfig& config)
    : workers_threads_(config.worker_threads),
      queue_semaphore_(kSemaphoreInitialCount, config.spinning_iterations) {}

void TaskQueue::Push(boost::intrusive_ptr<impl::TaskContext>&& context) {
  UASSERT(context);
  DoPush(context.get());
  context.detach();
}

boost::intrusive_ptr<impl::TaskContext> TaskQueue::PopBlocking() {
  // Current thread handles only a single TaskProcessor, so it's safe to store
  // a token for the task processor in a thread-local variable.
  thread_local moodycamel::ConsumerToken token(queue_);

  boost::intrusive_ptr<impl::TaskContext> context{DoPopBlocking(token),
                                                  /* add_ref= */ false};

  if (!context) {
    // return "stop" token back
    DoPush(nullptr);
  }

  return context;
}

void TaskQueue::StopProcessing() { DoPush(nullptr); }

std::size_t TaskQueue::GetSize() const noexcept { return queue_.size_approx(); }

std::size_t TaskQueue::GetSizeApproximate() const noexcept {
  return queue_size_cached_.load(std::memory_order_acquire);
}

void TaskQueue::DoPush(impl::TaskContext* context) {
  // This piece of code is copy-pasted from
  // moodycamel::BlockingConcurrentQueue::enqueue
  queue_.enqueue(context);
  queue_semaphore_.signal();
  UpdateQueueSize();
}

impl::TaskContext* TaskQueue::DoPopBlocking(moodycamel::ConsumerToken& token) {
  impl::TaskContext* context{};

  // This piece of code is copy-pasted from
  // moodycamel::BlockingConcurrentQueue::wait_dequeue
  queue_semaphore_.wait();
  while (!queue_.try_dequeue(token, context)) {
    // Can happen when another consumer steals our item in exchange for another
    // item in a Moodycamel sub-queue that we have already passed.
  }

  return context;
}

void TaskQueue::UpdateQueueSize() {
  if (utils::RandRange(workers_threads_) == 0) {
    queue_size_cached_.store(GetSize(), std::memory_order_relaxed);
  }
}

}  // namespace engine

USERVER_NAMESPACE_END
