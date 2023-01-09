#pragma once

#include <cstddef>
#include <variant>

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {
struct TaskProcessorConfig;
}  // namespace engine

namespace engine::impl {

class TaskContext;
class TaskQueuePinned;
class TaskQueueUnpinned;

class TaskQueue final {
 public:
  explicit TaskQueue(const TaskProcessorConfig& config);
  ~TaskQueue();

  void Push(TaskContext& context);

  // 'nullptr' means "the queue is shutting down".
  TaskContext* PopBlocking();

  // Must be called from each thread that will be working on the queue.
  void SetupThread() noexcept;

  // No new tasks should arrive after shutdown.
  void Shutdown() noexcept;

  std::size_t GetSizeApprox() const noexcept;

 private:
  utils::FastPimpl<std::variant<TaskQueuePinned, TaskQueueUnpinned>, 640, 8>
      impl_;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
