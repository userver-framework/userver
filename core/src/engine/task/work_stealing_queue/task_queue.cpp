#include <engine/task/work_stealing_queue/task_queue.hpp>

#include <engine/task/task_context.hpp>
#include <engine/task/work_stealing_queue/consumers_manager.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace {
// It is only used in worker threads outside of any coroutine,
// so it does not need to be protected via compiler::ThreadLocal
thread_local Consumer* localConsumer = nullptr;
}  // namespace

WorkStealingTaskQueue::WorkStealingTaskQueue(const TaskProcessorConfig& config)
    : consumers_manager_(config.worker_threads),
      consumers_count_(config.worker_threads),
      consumers_(config.worker_threads, *this, consumers_manager_) {
  for (size_t i = 0; i < consumers_count_; ++i) {
    consumers_[i].SetIndex(i);
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
  std::size_t size = global_queue_.GetSize();
  for (const auto& consumer : consumers_) {
    size += consumer.GetLocalQueueSize();
  }
  return size;
}

void WorkStealingTaskQueue::DoPush(impl::TaskContext* context) {
  if (context && context->IsBackground()) {
    background_queue_.Push(context);
  } else {
    Consumer* consumer = GetConsumer();
    if (consumer != nullptr && consumer->GetOwner() == this) {
      consumer->Push(context);
    } else {
      global_queue_.Push(context);
    }
  }
  consumers_manager_.NotifyNewTask();
}

impl::TaskContext* WorkStealingTaskQueue::DoPopBlocking() {
  Consumer* consumer = GetConsumer();
  if (consumer == nullptr) {
    DetermineConsumer();
    consumer = GetConsumer();
  }
  return consumer->PopBlocking();
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
