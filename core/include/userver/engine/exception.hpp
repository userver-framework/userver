#pragma once

/// @file engine/exception.hpp
/// @brief Coroutine engine exceptions

#include <stdexcept>

#include <engine/task/cancel.hpp>

namespace engine {

/// Wait interruption exception
class WaitInterruptedException : public std::runtime_error {
 public:
  explicit WaitInterruptedException(TaskCancellationReason reason)
      : std::runtime_error(
            "Wait interrupted because of task cancellation, reason=" +
            ToString(reason)),
        reason_(reason) {}

  TaskCancellationReason Reason() const { return reason_; }

 private:
  const TaskCancellationReason reason_;
};

/// Cancelled TaskWithResult access exception
class TaskCancelledException : public std::runtime_error {
 public:
  explicit TaskCancelledException(TaskCancellationReason reason)
      : std::runtime_error("Task cancelled, reason=" + ToString(reason)),
        reason_(reason) {}

  TaskCancellationReason Reason() const { return reason_; }

 private:
  const TaskCancellationReason reason_;
};

}  // namespace engine
