#pragma once

/// @file userver/engine/task/shared_task_with_result.hpp
/// @brief @copybrief engine::SharedTaskWithResult

#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <userver/engine/exception.hpp>
#include <userver/engine/impl/task_context_holder.hpp>
#include <userver/engine/task/shared_task.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/wrapped_call.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

// clang-format off

/// @brief Asynchronous task with result that has a shared ownership of payload
///
/// ## Example usage:
///
/// @snippet engine/task/shared_task_with_result_test.cpp Sample SharedTaskWithResult usage
///
/// @see @ref md_en_userver_synchronization

// clang-format on

template <typename T>
class [[nodiscard]] SharedTaskWithResult : public SharedTask {
 public:
  /// @brief Default constructor
  ///
  /// Creates an invalid task.
  SharedTaskWithResult() = default;

  /// @brief If the task is still valid and is not finished and this is the last
  /// shared owner of the payload, cancels the task and waits until it finishes.
  ~SharedTaskWithResult() = default;

  /// @brief Assigns the other task into this.
  SharedTaskWithResult(const SharedTaskWithResult& other) = default;

  /// @brief If this task is still valid and is not finished and other task is
  /// not the same task as this and this is the
  /// last shared owner of the payload, cancels the task and waits until it
  /// finishes before assigning the other. Otherwise just assigns the other task
  /// into this.
  SharedTaskWithResult& operator=(const SharedTaskWithResult& other) = default;

  /// @brief Moves the other task into this, leaving the other in an invalid
  /// state.
  SharedTaskWithResult(SharedTaskWithResult&& other) noexcept = default;

  /// @brief If this task is still valid and is not finished and other task is
  /// not the same task as this and this is the
  /// last shared owner of the payload, cancels the task and waits until it
  /// finishes before move assigning the other. Otherwise just move assigns the
  /// other task into this, leaving the other in an invalid state.
  SharedTaskWithResult& operator=(SharedTaskWithResult&& other) noexcept =
      default;

  /// @brief Returns (or rethrows) the result of task invocation.
  /// Task remains valid after return from this method,
  /// thread(coro)-safe.
  /// @returns const T& or void
  /// @throws WaitInterruptedException when `current_task::IsCancelRequested()`
  /// and no TaskCancellationBlockers are present.
  /// @throws TaskCancelledException
  ///   if no result is available because the task was cancelled
  decltype(auto) Get() const& noexcept(false) {
    UASSERT(IsValid());

    Wait();
    if (GetState() == State::kCancelled) {
      throw TaskCancelledException(CancellationReason());
    }

    return utils::impl::CastWrappedCall<T>(GetPayload()).Get();
  }

  std::add_lvalue_reference<const T> Get() && {
    static_assert(!sizeof(T*), "Store SharedTaskWithResult before using");
  }

  /// @cond
  static constexpr WaitMode kWaitMode = WaitMode::kMultipleWaiters;

  // For internal use only.
  explicit SharedTaskWithResult(impl::TaskContextHolder&& context)
      : SharedTask(std::move(context)) {}
  /// @endcond
};

}  // namespace engine

USERVER_NAMESPACE_END
