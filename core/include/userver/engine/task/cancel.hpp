#pragma once

#include <string>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <userver/engine/deadline.hpp>

/// @file userver/engine/task/cancel.hpp
/// @brief Task cancellation helpers

USERVER_NAMESPACE_BEGIN

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

class Task;
class TaskCancellationToken;

namespace current_task {

/// Checks for pending cancellation requests
bool IsCancelRequested() noexcept;

/// Checks for pending *non-blocked* cancellation requests
/// @sa TaskCancellationBlocker
bool ShouldCancel() noexcept;

/// Returns task cancellation reason for the current task
TaskCancellationReason CancellationReason() noexcept;

/// @brief \b Throws an exception if a cancellation request for this task is
/// pending.
///
/// @throws unspecified (non-std) exception if cancellation is pending and not
/// blocked
///
/// @warning cathching this exception without a rethrow in the same scope leads
/// to undefined behavior.
void CancellationPoint();

/// Set deadline for the current task.
/// The task will be cancelled when the deadline is reached.
void SetDeadline(Deadline deadline);

/// Return cancellation token for current coroutine.
TaskCancellationToken GetCancellationToken();

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
  impl::TaskContext& context_;
  const bool was_allowed_;
};

/// Returns a string representation of a cancellation reason
std::string ToString(TaskCancellationReason reason);

/// @brief Cancellation token to given task object
///
/// Unlike Task, TaskCancellationToken object doesn't wait for task finish in
/// its destructor. It is allowed to outlive the task object it was created
/// from. However, as long as there is any cancellation token associated with
/// given task, some internal structures of a task will not be freed.
///
/// General rule: whenever possible, prefer using engine::Task object instead.
class TaskCancellationToken final {
 public:
  /// Creates an invalid TaskCancellationToken
  TaskCancellationToken() noexcept;

  /// Creates a TaskCancellationToken associated with a task. The task must be
  /// valid.
  explicit TaskCancellationToken(Task& task);

  TaskCancellationToken(const TaskCancellationToken&) noexcept;
  TaskCancellationToken(TaskCancellationToken&&) noexcept;
  TaskCancellationToken& operator=(const TaskCancellationToken&) noexcept;
  TaskCancellationToken& operator=(TaskCancellationToken&&) noexcept;
  ~TaskCancellationToken();

  /// @see engine::Task::RequestCancel
  /// This method should not be called on invalid TaskCancellationToken
  void RequestCancel();

  /// True if this token is associated with a task
  bool IsValid() const noexcept;

 private:
  friend TaskCancellationToken current_task::GetCancellationToken();

  explicit TaskCancellationToken(impl::TaskContext& context) noexcept;

  boost::intrusive_ptr<impl::TaskContext> context_;
};

}  // namespace engine

USERVER_NAMESPACE_END
