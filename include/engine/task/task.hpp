#pragma once

/// @file engine/task/task.hpp
/// @brief @copybrief engine::Task

#include <chrono>
#include <string>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <engine/deadline.hpp>

namespace engine {
namespace ev {
class ThreadControl;
}  // namespace ev
namespace impl {
class TaskContext;
class TaskContextHolder;
}  // namespace impl

class TaskProcessor;

/// Asynchronous task
class Task {
 public:
  /// Task importance
  enum class Importance {
    /// Normal task
    kNormal,
    /// Critical task, cannot be cancelled due to task processor overload
    kCritical,
  };

  /// Task state
  enum class State {
    kInvalid,    ///< Unusable
    kNew,        ///< just created, not registered with task processor
    kQueued,     ///< awaits execution
    kRunning,    ///< executing user code
    kSuspended,  ///< suspended, e.g. waiting for blocking call to complete
    kCancelled,  ///< exited user code because of external request
    kCompleted,  ///< exited user code with return or throw
  };

  /// Task cancellation reason
  enum class CancellationReason {
    kNone,         ///< Not cancelled
    kUserRequest,  ///< User request
    kOverload,     ///< Task processor overload
    kAbandoned,    ///< Task destruction before finish
    kShutdown,     ///< Task processor shutdown
  };

  /// @brief Default constructor
  ///
  /// Creates an invalid task.
  Task();

  /// @brief Destructor
  ///
  /// When the task is still valid and is not finished, cancels it and waits
  /// until it finishes
  virtual ~Task();

  Task(Task&&) noexcept;
  Task& operator=(Task&&) noexcept;

  /// Checks whether the task is valid
  bool IsValid() const;
  explicit operator bool() const { return IsValid(); }

  /// Gets the task State
  State GetState() const;

  /// Returns whether the task finished execution
  bool IsFinished() const;

  /// @brief Suspends execution until the task finishes or caller is cancelled
  /// @throws WaitInterruptedException when `current_task::IsCancelRequested()`
  /// and no TaskCancellationBlockers are present.
  void Wait() const noexcept(false);

  /// @brief Suspends execution until the task finishes or after the specified
  /// timeout or until caller is cancelled
  /// @throws WaitInterruptedException when `current_task::IsCancelRequested()`
  /// and no TaskCancellationBlockers are present.
  template <typename Rep, typename Period>
  void WaitFor(const std::chrono::duration<Rep, Period>&) const noexcept(false);

  /// @brief Suspends execution until the task finishes or until the specified
  /// time point is reached or until caller is cancelled
  /// @throws WaitInterruptedException when `current_task::IsCancelRequested()`
  /// and no TaskCancellationBlockers are present.
  template <typename Clock, typename Duration>
  void WaitUntil(const std::chrono::time_point<Clock, Duration>&) const
      noexcept(false);

  /// @brief Detaches task, allowing it to continue execution out of scope
  /// @note After detach, Task becomes invalid
  void Detach() &&;

  /// Queues task cancellation request
  void RequestCancel();

  /// Gets task CancellationReason
  CancellationReason GetCancellationReason() const;

 protected:
  /// @cond
  /// Constructor for internal use
  Task(impl::TaskContextHolder&&);
  /// @endcond

 private:
  void DoWaitUntil(Deadline) const;
  void Terminate() noexcept;

  boost::intrusive_ptr<impl::TaskContext> context_;
};

/// Returns a string representation of a cancellation reason
std::string ToString(Task::CancellationReason reason);

/// Wait interruption exception
class WaitInterruptedException : public std::runtime_error {
 public:
  explicit WaitInterruptedException(Task::CancellationReason reason)
      : std::runtime_error(
            "Wait interrupted because of task cancellation, reason=" +
            ToString(reason)),
        reason_(reason) {}

  Task::CancellationReason Reason() const { return reason_; }

 private:
  const Task::CancellationReason reason_;
};

namespace current_task {

/// Returns reference to the task processor executing the caller
TaskProcessor& GetTaskProcessor();

/// @cond
/// Returns ev thread handle, internal use only
ev::ThreadControl& GetEventThread();

/// Updates spurious wakeup statistics, internal use only
void AccountSpuriousWakeup();
/// @endcond

}  // namespace current_task

template <typename Rep, typename Period>
void Task::WaitFor(const std::chrono::duration<Rep, Period>& duration) const
    noexcept(false) {
  DoWaitUntil(Deadline::FromDuration(duration));
}

template <typename Clock, typename Duration>
void Task::WaitUntil(const std::chrono::time_point<Clock, Duration>& until)
    const noexcept(false) {
  DoWaitUntil(Deadline::FromTimePoint(until));
}

}  // namespace engine
