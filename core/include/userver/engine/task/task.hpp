#pragma once

/// @file userver/engine/task/task.hpp
/// @brief @copybrief engine::Task

#include <userver/engine/task/task_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @brief Asynchronous task that has a unique ownership of the payload.
///
/// See engine::TaskWithResult for a type that could return a value or
/// report an exception from the payload.
class [[nodiscard]] Task : public TaskBase {
 public:
  /// @brief Default constructor
  ///
  /// Creates an invalid task.
  Task();

  /// @brief If the task is still valid and is not finished, cancels it and
  /// waits until it finishes.
  ~Task();

  /// @brief Moves the other task into this, leaving the other in an invalid
  /// state.
  Task(Task&& other) noexcept;

  /// @brief If this Task is still valid and is not finished, cancels it and
  /// waits until it finishes before moving the other. Otherwise just moves the
  /// other task into this, leaving the other in invalid state.
  Task& operator=(Task&& other) noexcept;

  Task(const Task&) = delete;
  Task& operator=(const Task&) = delete;

  /// @brief Detaches task, allowing it to continue execution out of scope;
  /// memory safety is much better with concurrent::BackgroundTaskStorage
  ///
  /// @note After detach, Task becomes invalid
  ///
  /// @warning Variables, which are captured by reference for this task in
  /// `Async*`, should outlive the task execution. This is hard to achieve in
  /// general, detached tasks may outlive all the components!
  /// Use concurrent::BackgroundTaskStorage as a safe and efficient alternative
  /// to calling Detach().
  void Detach() &&;

  /// @cond
  // For internal use only.
  impl::ContextAccessor* TryGetContextAccessor() noexcept;
  /// @endcond

 protected:
  /// @cond
  // For internal use only.
  explicit Task(impl::TaskContextHolder&& context);
  /// @endcond
};

}  // namespace engine

USERVER_NAMESPACE_END
