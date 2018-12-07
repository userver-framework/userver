#pragma once

/// @file engine/task/task_with_result.hpp
/// @brief @copybrief engine::TaskWithResult

#include <cassert>
#include <memory>
#include <stdexcept>

#include <engine/task/task.hpp>
#include <engine/task/task_context_holder.hpp>
#include <utils/wrapped_call.hpp>

namespace engine {

class TaskProcessor;

/// Cancelled TaskWithResult access exception
class TaskCancelledException : public std::runtime_error {
 public:
  explicit TaskCancelledException(Task::CancellationReason reason)
      : std::runtime_error("Task cancelled, reason=" + ToString(reason)),
        reason_(reason) {}

  Task::CancellationReason Reason() const { return reason_; }

 private:
  const Task::CancellationReason reason_;
};

/// Asynchronous task with result
template <typename T>
class TaskWithResult : public Task {
 public:
  /// @brief Default constructor
  ///
  /// Creates an invalid task.
  TaskWithResult() = default;

  /// @brief Constructor
  /// @param task_processor task processor used for execution of this task
  /// @param importance specifies whether this task can be auto-cancelled
  ///   in case of task processor overload
  /// @param wrapped_call_ptr task body
  /// @see Async()
  TaskWithResult(TaskProcessor& task_processor, Importance importance,
                 std::shared_ptr<utils::WrappedCall<T>>&& wrapped_call_ptr)
      : Task(impl::TaskContextHolder::MakeContext(
            task_processor, importance,
            [wrapped_call_ptr] { wrapped_call_ptr->Perform(); })),
        wrapped_call_ptr_(std::move(wrapped_call_ptr)) {}

  /// @brief Returns (or rethrows) the result of task invocation
  /// @throws WaitInterruptedException when `current_task::IsCancelRequested()`
  /// and no TaskCancellationBlockers are present.
  /// @throws TaskCancelledException
  ///   if no result is available becase the task was cancelled
  T Get() noexcept(false) {
    assert(wrapped_call_ptr_);
    Wait();
    if (GetState() == State::kCancelled) {
      throw TaskCancelledException(GetCancellationReason());
    }
    return wrapped_call_ptr_->Retrieve();
  }

 private:
  std::shared_ptr<utils::WrappedCall<T>> wrapped_call_ptr_;
};

}  // namespace engine
