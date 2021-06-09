#pragma once

/// @file engine/task/task_with_result.hpp
/// @brief @copybrief engine::TaskWithResult

#include <memory>
#include <stdexcept>

#include <engine/exception.hpp>
#include <engine/task/task.hpp>
#include <engine/task/task_context_holder.hpp>
#include <engine/task/task_processor_fwd.hpp>
#include <utils/assert.hpp>
#include <utils/clang_format_workarounds.hpp>
#include <utils/wrapped_call.hpp>

namespace engine {

/// Asynchronous task with result
///
/// ## Example usage:
///
/// @snippet engine/task/task_with_result_test.cpp  Sample TaskWithResult usage
///
/// @see @ref md_en_userver_synchronization
template <typename T>
class USERVER_NODISCARD TaskWithResult : public Task {
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
  TaskWithResult(
      TaskProcessor& task_processor, Importance importance,
      std::shared_ptr<utils::impl::WrappedCall<T>>&& wrapped_call_ptr)
      : Task(impl::TaskContextHolder::MakeContext(
            task_processor, importance,
            [wrapped_call_ptr] { wrapped_call_ptr->Perform(); })),
        wrapped_call_ptr_(std::move(wrapped_call_ptr)) {}

  /// @brief Returns (or rethrows) the result of task invocation.
  /// After return from this method the task is not valid.
  /// @throws WaitInterruptedException when `current_task::IsCancelRequested()`
  /// and no TaskCancellationBlockers are present.
  /// @throws TaskCancelledException
  ///   if no result is available because the task was cancelled
  T Get() noexcept(false) {
    UASSERT(wrapped_call_ptr_);
    Wait();
    if (GetState() == State::kCancelled) {
      throw TaskCancelledException(CancellationReason());
    }
    Invalidate();
    return std::exchange(wrapped_call_ptr_, nullptr)->Retrieve();
  }

 private:
  std::shared_ptr<utils::impl::WrappedCall<T>> wrapped_call_ptr_;
};

}  // namespace engine
