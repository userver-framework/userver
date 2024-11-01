#pragma once

/// @file userver/engine/task/task_base.hpp
/// @brief @copybrief engine::TaskBase

#include <chrono>
#include <memory>
#include <string_view>

#include <userver/engine/deadline.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/fast_pimpl.hpp>

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

/// @brief Base class for all the asynchronous tasks
/// (engine::Task, engine::SharedTask, engine::SharedTaskWithResult,
/// engine::TaskWithResult, dist_lock::DistLockedTask, ...).
class [[nodiscard]] TaskBase {
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

    /// @brief Checks whether this object owns
    /// an actual task (not `State::kInvalid`)
    ///
    /// An invalid task cannot be used. The task becomes invalid
    /// after each of the following calls:
    ///
    /// 1. the default constructor
    /// 2. `Detach()`
    /// 3. `Get()` (see `engine::TaskWithResult`)
    bool IsValid() const;

    /// Gets the task State
    State GetState() const;

    static std::string_view GetStateName(State state);

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
    void WaitUntil(const std::chrono::time_point<Clock, Duration>&) const noexcept(false);

    /// @brief Suspends execution until the task finishes or until the specified
    /// deadline is reached or until caller is cancelled
    /// @throws WaitInterruptedException when `current_task::IsCancelRequested()`
    /// and no TaskCancellationBlockers are present.
    void WaitUntil(Deadline) const;

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
    // For internal use only.
    TaskBase();

    // For internal use only.
    explicit TaskBase(impl::TaskContextHolder&& context);

    // The following special functions must remain protected to forbid slicing
    // and force those methods implementation in derived classes.
    TaskBase(TaskBase&&) noexcept;
    TaskBase& operator=(TaskBase&&) noexcept;
    TaskBase(const TaskBase&) noexcept;
    TaskBase& operator=(const TaskBase&) noexcept;
    ~TaskBase();

    // For internal use only.
    impl::TaskContext& GetContext() const noexcept;

    // For internal use only.
    bool HasSameContext(const TaskBase& other) const noexcept;

    // For internal use only.
    utils::impl::WrappedCallBase& GetPayload() const noexcept;

    // Marks task as invalid. For internal use only.
    void Invalidate() noexcept;

    void Terminate(TaskCancellationReason) noexcept;
    /// @endcond

private:
    friend class impl::DetachedTasksSyncBlock;
    friend class TaskCancellationToken;

    struct Impl;
    utils::FastPimpl<Impl, 8, 8> pimpl_;
};

/// @brief Namespace with functions to work with current task from within it
namespace current_task {

/// Returns true only when running in userver coroutine environment,
/// i.e. in an engine::TaskProcessor thread.
bool IsTaskProcessorThread() noexcept;

/// Returns reference to the task processor executing the caller
TaskProcessor& GetTaskProcessor();

/// Returns task coroutine stack size
std::size_t GetStackSize();

/// @cond
// Returns ev thread handle, internal use only
ev::ThreadControl& GetEventThread();
/// @endcond

}  // namespace current_task

template <typename Rep, typename Period>
void TaskBase::WaitFor(const std::chrono::duration<Rep, Period>& duration) const noexcept(false) {
    WaitUntil(Deadline::FromDuration(duration));
}

template <typename Clock, typename Duration>
void TaskBase::WaitUntil(const std::chrono::time_point<Clock, Duration>& until) const noexcept(false) {
    WaitUntil(Deadline::FromTimePoint(until));
}

namespace impl {

// For internal use only.
std::uint64_t GetCreatedTaskCount(TaskProcessor&);

}  // namespace impl

}  // namespace engine

USERVER_NAMESPACE_END
