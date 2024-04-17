#include "task_queue.hpp"
#include <engine/task/task_context.hpp>
#include "consumers_manager.hpp"

namespace {
thread_local userver::engine::Consumer* localConsumer = nullptr;
}

USERVER_NAMESPACE_BEGIN

namespace engine {

WorkStealingTaskQueue::WorkStealingTaskQueue(const TaskProcessorConfig& config)
    : consumers_manager_(config.worker_threads),
      consumers_count_(config.worker_threads),
      consumers_order_(0) {
  consumers_.reserve(consumers_count_);
  for (size_t i = 0; i < consumers_count_; ++i) {
    consumers_.emplace_back(this, &consumers_manager_, i);
  }
}

void WorkStealingTaskQueue::Push(
    boost::intrusive_ptr<impl::TaskContext>&& context) {
  DoPush(context.get());
  context.detach();
}

boost::intrusive_ptr<impl::TaskContext> WorkStealingTaskQueue::PopBlocking() {
  boost::intrusive_ptr<impl::TaskContext> context{DoPopBlocking(),
                                                  /* add_ref= */ false};
  if (!context) {
    DoPush(nullptr);
  }

  return context;
}

void WorkStealingTaskQueue::StopProcessing() { consumers_manager_.Stop(); }

std::size_t WorkStealingTaskQueue::GetSizeApproximate() const noexcept {
  std::size_t size = global_queue_.Size();
  for (const auto& consumer : consumers_) {
    size += consumer.LocalQueueSize();
  }
  return size;
}

void WorkStealingTaskQueue::DoPush(impl::TaskContext* context) {
  Consumer* consumer = GetConsumer();
  if (consumer == nullptr || consumer->GetOwner() != this ||
      !consumer->Push(context)) {
    global_queue_.Push(context);
  }
  consumers_manager_.NotifyNewTask();
}

impl::TaskContext* WorkStealingTaskQueue::DoPopBlocking() {
  Consumer* consumer = GetConsumer();
  if (consumer == nullptr) {
    DetermineConsumer();
    consumer = GetConsumer();
  }
  return consumer->Pop();
}

Consumer* WorkStealingTaskQueue::GetConsumer() { return localConsumer; }

void WorkStealingTaskQueue::DetermineConsumer() {
  std::size_t index = consumers_order_.load();
  while (!consumers_order_.compare_exchange_weak(index, index + 1)) {
  }
  if (index < consumers_count_) {
    localConsumer = &consumers_[index % consumers_count_];
  }
}

void WorkStealingTaskQueue::DropConsumer() { localConsumer = nullptr; }

}  // namespace engine

USERVER_NAMESPACE_END
