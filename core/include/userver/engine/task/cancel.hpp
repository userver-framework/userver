#pragma once

#include <string>

#include <userver/engine/deadline.hpp>

/// @file userver/engine/task/cancel.hpp
/// @brief Task cancellation helpers

namespace engine {
namespace impl {
class TaskContext;
}  // namespace impl

/// Task cancellation reason
enum class TaskCancellationReason {
  kNone,         ///< Not cancelled
  kUserRequest,  ///< User request
  kDeadline,     ///< Deadline
  kOverload,     ///< Task processor overload
  kAbandoned,    ///< Task destruction before finish
  kShutdown,     ///< Task processor shutdown
};

namespace current_task {

/// Checks for pending cancellation requests
bool IsCancelRequested() noexcept;

/// Checks for pending *non-blocked* cancellation requests
/// @sa TaskCancellationBlocker
bool ShouldCancel() noexcept;

/// Returns task cancellation reason for the current task
TaskCancellationReason CancellationReason();

/// @brief \b Throws an exception if a cancellation request for this task is
/// pending.
///
/// @throws unspecified (non-std) exception if cancellation is pending and not
/// blocked
///
/// @warning cathching this exception whithout a rethrow in the same scope leads
/// to undefined behavior.
void CancellationPoint();

/// Set deadline for the current task.
/// The task will be cancelled when the deadline is reached.
void SetDeadline(Deadline deadline);

}  // namespace current_task

/// Blocks cancellation for specific scopes, e.g. destructors.
/// Recursive, i.e. can be instantiated multiple times in a given call stack.
class TaskCancellationBlocker final {
 public:
  TaskCancellationBlocker();
  ~TaskCancellationBlocker();

  TaskCancellationBlocker(const TaskCancellationBlocker&) = delete;
  TaskCancellationBlocker(TaskCancellationBlocker&&) = delete;
  TaskCancellationBlocker& operator=(const TaskCancellationBlocker&) = delete;
  TaskCancellationBlocker& operator=(TaskCancellationBlocker&&) = delete;

 private:
  impl::TaskContext* const context_;
  const bool was_allowed_;
};

/// Returns a string representation of a cancellation reason
std::string ToString(TaskCancellationReason reason);

}  // namespace engine
