#include <engine/task/task_queue.hpp>

#include <moodycamel/blockingconcurrentqueue.h>

#include <engine/task/task_context.hpp>
#include <engine/task/task_processor_config.hpp>
#include <engine/task/task_queue_pinned.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskQueueUnpinned final {
 public:
  TaskQueueUnpinned() = default;

  void Push(TaskContext& context) { queue_.enqueue(&context); }

  TaskContext* PopBlocking() {
    thread_local moodycamel::ConsumerToken consumer_token(queue_);

    TaskContext* result = nullptr;
    queue_.wait_dequeue(consumer_token, result);

    if (result == nullptr) {
      // Put back the shutdown marker.
      queue_.enqueue(nullptr);
    }
    return result;
  }

  void SetupThread() noexcept {}

  void Shutdown() noexcept { queue_.enqueue(nullptr); }

  std::size_t GetSizeApprox() const noexcept { return queue_.size_approx(); }

 private:
  moodycamel::BlockingConcurrentQueue<TaskContext*> queue_;
};

namespace {

using TaskQueueImpl =
    utils::FastPimpl<std::variant<TaskQueuePinned, TaskQueueUnpinned>, 640, 8>;

TaskQueueImpl MakeTaskQueue(const TaskProcessorConfig& config) {
  switch (config.task_thread_switches) {
    case TaskThreadSwitches::kAllowed:
      return TaskQueueImpl(std::in_place_type<TaskQueueUnpinned>);
    case TaskThreadSwitches::kNotAllowed:
      return TaskQueueImpl(std::in_place_type<TaskQueuePinned>, config);
  }
  UINVARIANT(false, "Invalid value of TaskThreadSwitches enum");
}

}  // namespace

TaskQueue::TaskQueue(const userver::engine::TaskProcessorConfig& config)
    : impl_(MakeTaskQueue(config)) {}

TaskQueue::~TaskQueue() = default;

void TaskQueue::Push(TaskContext& context) {
  std::visit([&](auto& impl) { impl.Push(context); }, *impl_);
}

TaskContext* TaskQueue::PopBlocking() {
  return std::visit([](auto& impl) { return impl.PopBlocking(); }, *impl_);
}

void TaskQueue::SetupThread() noexcept {
  std::visit([&](auto& impl) { impl.SetupThread(); }, *impl_);
}

void TaskQueue::Shutdown() noexcept {
  std::visit([&](auto& impl) { impl.Shutdown(); }, *impl_);
}

std::size_t TaskQueue::GetSizeApprox() const noexcept {
  return std::visit([](auto& impl) { return impl.GetSizeApprox(); }, *impl_);
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
