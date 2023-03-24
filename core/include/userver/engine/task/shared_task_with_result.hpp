#pragma once

/// @file userver/engine/task/shared_task_with_result.hpp
/// @brief @copybrief engine::SharedTaskWithResult

#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <userver/engine/exception.hpp>
#include <userver/engine/impl/task_context_holder.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/assert.hpp>
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
class [[nodiscard]] SharedTaskWithResult : public Task {
 public:
  /// @brief Default constructor
  ///
  /// Creates an invalid task.
  SharedTaskWithResult() = default;

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
      : Task(std::move(context)) {}
  /// @endcond
};

}  // namespace engine

USERVER_NAMESPACE_END
