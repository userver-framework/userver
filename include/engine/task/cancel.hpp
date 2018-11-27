#pragma once

/// @file engine/task/cancel.hpp
/// @brief Task cancellation helpers

namespace engine {
namespace impl {
class TaskContext;
}  // namespace impl

namespace current_task {

/// Checks for pending cancellation requests
bool IsCancelRequested();

/// @brief Cancels the current task if the cancellation request is pending
/// @throws unspecified exception if cancellation is pending and not blocked
/// @warning swallowing this exception leads to undefined behavior
void CancellationPoint();

}  // namespace current_task

/// Blocks cancellation for specific scopes, e.g. destructors
class TaskCancellationBlocker {
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

}  // namespace engine
