#pragma once

/// @file userver/engine/task/task_with_result.hpp
/// @brief @copybrief engine::TaskWithResult

#include <memory>
#include <stdexcept>
#include <utility>

#include <userver/engine/exception.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/clang_format_workarounds.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/impl/wrapped_call.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace impl {
class TaskFactory;
}

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

  /*/// @brief Constructor, for internal use only
  /// @param task_processor task processor used for execution of this task
  /// @param importance specifies whether this task can be auto-cancelled
  ///   in case of task processor overload
  /// @param wrapped_call_ptr task body
  /// @see Async()
  TaskWithResult(
      TaskProcessor& task_processor, Task::Importance importance,
      Deadline deadline,
      std::unique_ptr<utils::impl::WrappedCall<T>>&& wrapped_call_ptr)
      : Task(task_processor, importance, Task::WaitMode::kSingleWaiter,
             deadline, std::move(wrapped_call_ptr)) {}*/

  TaskWithResult(const TaskWithResult&) = delete;
  TaskWithResult& operator=(const TaskWithResult&) = delete;

  TaskWithResult(TaskWithResult&&) noexcept = default;
  TaskWithResult& operator=(TaskWithResult&&) noexcept = default;

  /// @brief Returns (or rethrows) the result of task invocation.
  /// After return from this method the task is not valid.
  /// @throws WaitInterruptedException when `current_task::IsCancelRequested()`
  /// and no TaskCancellationBlockers are present.
  /// @throws TaskCancelledException
  ///   if no result is available because the task was cancelled
  T Get() noexcept(false) {
    EnsureValid();

    Wait();
    if (GetState() == State::kCancelled) {
      throw TaskCancelledException(CancellationReason());
    }

    utils::FastScopeGuard invalidate([this]() noexcept { Invalidate(); });
    return utils::impl::CastWrappedCall<T>(GetPayload()).Retrieve();
  }

  using Task::TryGetContextAccessor;

 private:
  friend class impl::TaskFactory;

  TaskWithResult(
      TaskProcessor& task_processor, Task::Importance importance,
      Deadline deadline, std::unique_ptr<char[]>&& context_storage, utils::impl::WrappedCallBase* payload)
      : Task(task_processor, importance, Task::WaitMode::kSingleWaiter,
             deadline, std::move(context_storage), payload) {}

  void EnsureValid() const {
    UINVARIANT(IsValid(),
               "TaskWithResult::Get was called on an invalid task. Note that "
               "Get invalidates self, so it must be called at most once "
               "per task");
  }
};

}  // namespace engine

USERVER_NAMESPACE_END
