#include <engine/task/work_stealing_queue/task_queue.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/rand.hpp>

#include <engine/task/task_context.hpp>
#include <engine/task/work_stealing_queue/consumer.hpp>
#include <engine/task/work_stealing_queue/consumers_manager.hpp>
#include <engine/task/work_stealing_queue/global_queue.hpp>
#include <iostream>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace {
// It is only used in worker threads outside of any coroutine,
// so it does not need to be protected via compiler::ThreadLocal
thread_local Consumer* localConsumer = nullptr;
}  // namespace

WorkStealingTaskQueue::WorkStealingTaskQueue(const TaskProcessorConfig& config)
    : consumers_count_(config.worker_threads),
      global_queue_(consumers_count_),
      background_queue_(consumers_count_),
      consumers_(config.worker_threads, *this, consumers_manager_),
      consumers_manager_(consumers_count_) {
  for (size_t i = 0; i < consumers_count_; ++i) {
    consumers_[i].SetIndex(i);
  }
}

WorkStealingTaskQueue::~WorkStealingTaskQueue() {
  // for (std::size_t i = 0; i < consumers_count_; i++) {
  //   std::cout << consumers_[i].sleep_counter_ << "\n";
  // }
  std::cout << "-------\n" << std::flush;
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

std::size_t WorkStealingTaskQueue::GetSize() const noexcept {
  std::size_t size{0};
  for (const auto& consumer : consumers_) {
    size += consumer.GetLocalQueueSize();
  }
  size += global_queue_.GetSizeApproximate();
  size += background_queue_.GetSizeApproximate();
  return size;
}

std::size_t WorkStealingTaskQueue::GetSizeApproximate() const noexcept {
  return queue_size_cached_.load(std::memory_order_relaxed);
}

void WorkStealingTaskQueue::PrepareWorker(std::size_t index) {
  if (index < consumers_count_) {
    localConsumer = &consumers_[index];
  }
}

void WorkStealingTaskQueue::DoPush(impl::TaskContext* context) {
  {
    Consumer* consumer = GetConsumer();
    if (consumer != nullptr && consumer->GetOwner() == this) {
      consumer->Push(context);
    } else if (context && context->IsBackground()) {
      background_queue_.Push(context);
    } else {
      global_queue_.Push(context);
    }
  }
  consumers_manager_.NotifyNewTask();
}

impl::TaskContext* WorkStealingTaskQueue::DoPopBlocking() {
  Consumer* consumer = GetConsumer();
  UASSERT(consumer != nullptr);
  return consumer->PopBlocking();
}

Consumer* WorkStealingTaskQueue::GetConsumer() { return localConsumer; }

void WorkStealingTaskQueue::UpdateQueueSize() {
  if (utils::RandRange(consumers_count_) == 0) {
    queue_size_cached_.store(GetSize(), std::memory_order_relaxed);
  }
}

}  // namespace engine

USERVER_NAMESPACE_END