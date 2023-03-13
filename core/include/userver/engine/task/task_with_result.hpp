#pragma once

/// @file userver/engine/task/task_with_result.hpp
/// @brief @copybrief engine::TaskWithResult

#include <memory>
#include <stdexcept>
#include <utility>

#include <userver/engine/exception.hpp>
#include <userver/engine/impl/task_context_holder.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/impl/wrapped_call.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// Asynchronous task with result
///
/// ## Example usage:
///
/// @snippet engine/task/task_with_result_test.cpp  Sample TaskWithResult usage
///
/// @see @ref md_en_userver_synchronization
template <typename T>
class [[nodiscard]] TaskWithResult : public Task {
 public:
  /// @brief Default constructor
  ///
  /// Creates an invalid task.
  TaskWithResult() = default;

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

  /// @cond
  static constexpr WaitMode kWaitMode = WaitMode::kSingleWaiter;

  // For internal use only.
  explicit TaskWithResult(impl::TaskContextHolder&& context)
      : Task(std::move(context)) {}
  /// @endcond

 private:
  void EnsureValid() const {
    UINVARIANT(IsValid(),
               "TaskWithResult::Get was called on an invalid task. Note that "
               "Get invalidates self, so it must be called at most once "
               "per task");
  }
};

}  // namespace engine

USERVER_NAMESPACE_END
