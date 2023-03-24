#pragma once

/// @file userver/engine/task/task.hpp
/// @brief @copybrief engine::Task

#include <chrono>
#include <memory>
#include <string>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <userver/engine/deadline.hpp>
#include <userver/engine/exception.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {
class WrappedCallBase;
}  // namespace utils::impl

namespace engine {
namespace ev {
class ThreadControl;
}  // namespace ev
namespace impl {
class TaskContextHolder;
class TaskContext;
class DetachedTasksSyncBlock;
class ContextAccessor;
}  // namespace impl

/// Asynchronous task
class [[nodiscard]] Task {
 public:
  /// Task importance
  enum class Importance {
    /// Normal task
    kNormal,

    /// Critical task. The task will be started regardless of cancellations,
    /// e.g. due to user request, deadline or TaskProcessor overload. After the
    /// task starts, it may be cancelled. In particular, if it received any
    /// cancellation requests before starting, then it will start as cancelled.
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

  /// Task wait mode
  enum class WaitMode {
    /// Can be awaited by at most one task at a time
    kSingleWaiter,
    /// Can be awaited by multiple tasks simultaneously
    kMultipleWaiters
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

  /// @brief Checks whether this `Task` object owns
  /// an actual task (not `State::kInvalid`)
  ///
  /// An invalid task cannot be used. The task becomes invalid
  /// after each of the following calls:
  ///
  /// 1. `Task()`, the default constructor
  /// 2. `Detach()`
  /// 3. `Get()` (see `engine::TaskWithResult`)
  bool IsValid() const;

  /// Gets the task State
  State GetState() const;

  static const std::string& GetStateName(State state);

  /// Returns whether the task finished execution
  bool IsFinished() const;

  /// @brief Suspends execution until the task finishes or caller is cancelled.
  /// Can be called from coroutine context only. For non-coroutine context use
  /// BlockingWait().
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

  /// @brief Suspends execution until the task finishes or until the specified
  /// deadline is reached or until caller is cancelled
  /// @throws WaitInterruptedException when `current_task::IsCancelRequested()`
  /// and no TaskCancellationBlockers are present.
  void WaitUntil(Deadline) const;

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

  /// Queues task cancellation request
  void RequestCancel();

  /// @brief Cancels the task and suspends execution until it is finished.
  /// Can be called from coroutine context only. For non-coroutine context use
  /// RequestCancel() + BlockingWait().
  void SyncCancel() noexcept;

  /// Gets task cancellation reason
  TaskCancellationReason CancellationReason() const;

  /// Waits for the task in non-coroutine context
  /// (e.g. non-TaskProcessor's std::thread).
  void BlockingWait() const;

 protected:
  /// @cond
  Task(const Task&);
  Task& operator=(const Task&);

  // For internal use only.
  explicit Task(impl::TaskContextHolder&& context);

  // Marks task as invalid. For internal use only.
  void Invalidate() noexcept;

  // For internal use only.
  utils::impl::WrappedCallBase& GetPayload() const noexcept;

  // Internal helper for WaitAny/WaitAll.
  impl::ContextAccessor* TryGetContextAccessor() noexcept;
  /// @endcond

 private:
  friend class impl::DetachedTasksSyncBlock;
  friend class TaskCancellationToken;

  bool IsSharedWaitAllowed() const;

  void Terminate(TaskCancellationReason) noexcept;

  boost::intrusive_ptr<impl::TaskContext> context_;
};

namespace current_task {

/// Returns reference to the task processor executing the caller
TaskProcessor& GetTaskProcessor();

/// Returns a pointer to the task processor executing the caller, or nullptr if
/// running outside of coroutine environment
TaskProcessor* GetTaskProcessorOptional() noexcept;

/// @cond
/// Returns ev thread handle, internal use only
ev::ThreadControl& GetEventThread();

/// Updates spurious wakeup statistics, internal use only
void AccountSpuriousWakeup();
/// @endcond

/// Returns task coroutine stack size
size_t GetStackSize();

}  // namespace current_task

template <typename Rep, typename Period>
void Task::WaitFor(const std::chrono::duration<Rep, Period>& duration) const
    noexcept(false) {
  WaitUntil(Deadline::FromDuration(duration));
}

template <typename Clock, typename Duration>
void Task::WaitUntil(const std::chrono::time_point<Clock, Duration>& until)
    const noexcept(false) {
  WaitUntil(Deadline::FromTimePoint(until));
}

}  // namespace engine

USERVER_NAMESPACE_END
