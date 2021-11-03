#pragma once

/// @file userver/engine/exception.hpp
/// @brief Coroutine engine exceptions

#include <stdexcept>

#include <userver/engine/task/cancel.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// Thrown if a wait operation on the current task has been interrupted, usually
/// due to a timeout or cancellation
class WaitInterruptedException : public std::runtime_error {
 public:
  explicit WaitInterruptedException(TaskCancellationReason reason);

  TaskCancellationReason Reason() const noexcept;

 private:
  const TaskCancellationReason reason_;
};

/// Thrown if a `TaskWithResult`, for which we were waiting, got cancelled
class TaskCancelledException : public std::runtime_error {
 public:
  explicit TaskCancelledException(TaskCancellationReason reason);

  TaskCancellationReason Reason() const noexcept;

 private:
  const TaskCancellationReason reason_;
};

}  // namespace engine

USERVER_NAMESPACE_END
