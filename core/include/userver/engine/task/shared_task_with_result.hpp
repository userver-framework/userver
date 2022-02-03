#pragma once

/// @file userver/engine/task/shared_task_with_result.hpp
/// @brief @copybrief engine::SharedTaskWithResult

#include <memory>
#include <stdexcept>
#include <type_traits>

#include <userver/engine/exception.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/engine/task/task_context_holder.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/clang_format_workarounds.hpp>
#include <userver/utils/impl/wrapped_call.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

// clang-format off

/// Asynchronous task with result
///
/// ## Example usage:
///
/// @snippet engine/task/shared_task_with_result_test.cpp Sample SharedTaskWithResult usage
///
/// @see @ref md_en_userver_synchronization

// clang-format on

template <typename T>
class USERVER_NODISCARD SharedTaskWithResult : public Task {
 public:
  /// @brief Default constructor
  ///
  /// Creates an invalid task.
  SharedTaskWithResult() = default;

  /// @brief Constructor
  /// @param task_processor task processor used for execution of this task
  /// @param importance specifies whether this task can be auto-cancelled
  ///   in case of task processor overload
  /// @param wrapped_call_ptr task body
  /// @see SharedAsync()
  SharedTaskWithResult(
      TaskProcessor& task_processor, Task::Importance importance,
      Deadline deadline,
      std::shared_ptr<utils::impl::WrappedCall<T>>&& wrapped_call_ptr)
      : Task(impl::TaskContextHolder::MakeContext(
            task_processor, importance, Task::WaitMode::kMultipleWaiters,
            deadline, impl::Payload(wrapped_call_ptr))),
        wrapped_call_ptr_(std::move(wrapped_call_ptr)) {}

  /// @brief Returns (or rethrows) the result of task invocation.
  /// Task remains valid after return from this method,
  /// thread(coro)-safe.
  /// @throws WaitInterruptedException when `current_task::IsCancelRequested()`
  /// and no TaskCancellationBlockers are present.
  /// @throws TaskCancelledException
  ///   if no result is available because the task was cancelled
  std::add_lvalue_reference_t<const T> Get() const& noexcept(false) {
    UASSERT(this->wrapped_call_ptr_);

    Wait();
    if (GetState() == State::kCancelled) {
      throw TaskCancelledException(CancellationReason());
    }

    return wrapped_call_ptr_->Get();
  }

  std::add_lvalue_reference<const T> Get() && {
    static_assert(!sizeof(T*), "Store SharedTaskWithResult before using");
  }

 private:
  std::shared_ptr<utils::impl::WrappedCall<T>> wrapped_call_ptr_;
};

}  // namespace engine

USERVER_NAMESPACE_END
