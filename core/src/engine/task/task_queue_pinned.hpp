#pragma once

#include <atomic>
#include <cstddef>

#include <userver/utils/fixed_array.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {
struct TaskProcessorConfig;
}  // namespace engine

namespace engine::impl {

class TaskContext;

class TaskQueuePinned final {
 public:
  explicit TaskQueuePinned(const TaskProcessorConfig& config);
  ~TaskQueuePinned();

  void Push(TaskContext& context);

  TaskContext* PopBlocking();

  void SetupThread() noexcept;

  void Shutdown() noexcept;

  std::size_t GetSizeApprox() const noexcept;

 private:
  struct SubQueue;

  std::size_t FindLeastOccupiedThread() const noexcept;

  utils::FixedArray<SubQueue> sub_queues_;
  std::atomic<std::size_t> next_unowned_sub_queue_{0};
  std::atomic<bool> is_shut_down_{false};
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
