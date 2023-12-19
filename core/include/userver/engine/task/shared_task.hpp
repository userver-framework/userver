#pragma once

/// @file userver/engine/task/shared_task.hpp
/// @brief @copybrief engine::SharedTask

#include <memory>
#include <type_traits>
#include <utility>

#include <userver/engine/impl/task_context_holder.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @brief Asynchronous task that has a shared ownership of the payload.
///
/// See engine::SharedTaskWithResult for a type that could return a value or
/// report an exception from the payload.
class [[nodiscard]] SharedTask : public TaskBase {
 public:
  /// @brief Default constructor
  ///
  /// Creates an invalid task.
  SharedTask();

  /// @brief If the task is still valid and is not finished and this is the last
  /// shared owner of the payload, cancels the task and waits until it finishes.
  ~SharedTask();

  /// @brief Assigns the other task into this.
  SharedTask(const SharedTask& other) noexcept;

  /// @brief If this task is still valid and is not finished and other task is
  /// not the same task as this and this is the
  /// last shared owner of the payload, cancels the task and waits until it
  /// finishes before assigning the other. Otherwise just assigns the other task
  /// into this.
  SharedTask& operator=(const SharedTask& other) noexcept;

  /// @brief Moves the other task into this, leaving the other in an invalid
  /// state.
  SharedTask(SharedTask&& other) noexcept;

  /// @brief If this task is still valid and is not finished and other task is
  /// not the same task as this and this is the
  /// last shared owner of the payload, cancels the task and waits until it
  /// finishes before move assigning the other. Otherwise just move assigns the
  /// other task into this, leaving the other in an invalid state.
  SharedTask& operator=(SharedTask&& other) noexcept;

  /// @cond
  static constexpr WaitMode kWaitMode = WaitMode::kMultipleWaiters;

 protected:
  // For internal use only.
  explicit SharedTask(impl::TaskContextHolder&& context);
  /// @endcond

 private:
  void DecrementSharedUsages() noexcept;
  void IncrementSharedUsages() noexcept;
};

}  // namespace engine

USERVER_NAMESPACE_END
