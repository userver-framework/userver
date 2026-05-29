#pragma once

#include <atomic>
#include <cstdint>

#include <ev.h>

#include <engine/coro/pool.hpp>
#include <engine/ev/thread_control.hpp>
#include <engine/task/context_timer.hpp>
#include <engine/task/counted_coroutine_ptr.hpp>
#include <engine/task/cxxabi_eh_globals.hpp>
#include <engine/task/sleep_state.hpp>
#include <engine/task/task_counter.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/future_status.hpp>
#include <userver/engine/impl/actor.hpp>
#include <userver/engine/impl/awaiter.hpp>
#include <userver/engine/impl/context_accessor.hpp>
#include <userver/engine/impl/detached_tasks_sync_block.hpp>
#include <userver/engine/impl/task_local_storage.hpp>
#include <userver/engine/impl/wait_list_fwd.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/flags.hpp>
#include <userver/utils/impl/wrapped_call_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {
namespace impl {

class TaskContextHolder;

[[noreturn]] void ReportDeadlock();

class WaitStrategy {
public:
    // Implementation may set up timers/watchers here. Implementation must make
    // sure that there is no race between SetupWakeups() and WaitList-specific
    // wakeup (if "add task to wait list iff not ready" is not protected from
    // Wakeup, e.g. for WaitListLight). SetupWakeups() *may* call Wakeup() for
    // current task - sleep_state_ is set in DoStep() and double-checked for such
    // early wakeups. It may not sleep.
    //
    // If EarlyWakeup{true} is returned, then:
    // - DisableWakeups is not called;
    // - SetupWakeups should disable wakeup sources itself;
    // - SetupWakeups may or may not call context.Wakeup.
    virtual EarlyNotify SetupWakeups() = 0;

    // Implementation must disable all wakeup sources (wait lists, timers) here.
    // It may not sleep.
    virtual void DisableWakeups() noexcept = 0;

protected:
    constexpr WaitStrategy() noexcept = default;

    // Prevent destruction via pointer to base.
    ~WaitStrategy() = default;
};

// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class TaskContext final : public ContextAccessor, public Awaiter, public deadlock_detector::Actor {
public:
    using TaskPipe = coro::Pool::TaskPipe;
    using TaskId = uint64_t;

    enum class YieldReason { kNone, kTaskWaiting, kTaskCancelled, kTaskComplete };

    /// Wakeup sources in descending priority order
    enum class WakeupSource : uint32_t {
        kNone = static_cast<uint32_t>(SleepFlags::kNone),
        kNotify = static_cast<uint32_t>(SleepFlags::kWakeupByWaitList),
        kDeadlineTimer = static_cast<uint32_t>(SleepFlags::kWakeupByDeadlineTimer),
        kCancelRequest = static_cast<uint32_t>(SleepFlags::kWakeupByCancelRequest),
        kBootstrap = static_cast<uint32_t>(SleepFlags::kWakeupByBootstrap),
    };

    TaskContext(TaskProcessor&, Task::Importance, Task::WaitMode, Deadline, utils::impl::WrappedCallBase& payload);

    ~TaskContext() noexcept;

    TaskContext(const TaskContext&) = delete;
    TaskContext(TaskContext&&) = delete;
    TaskContext& operator=(const TaskContext&) = delete;
    TaskContext& operator=(TaskContext&&) = delete;

    // can only be called on a State::kCompleted task
    utils::impl::WrappedCallBase& GetPayload() noexcept;

    Task::State GetState() const { return state_; }

    // whether this task is the one currently executing on the calling thread
    bool IsCurrent() const noexcept;

    // whether task respects task processor queue size limits
    // exceeding these limits causes task to become cancelled
    bool IsCritical() const;

    // whether task is allowed to be awaited from multiple coroutines
    // simultaneously
    bool IsSharedWaitAllowed() const;

    // whether user code finished executing, coroutine may still be running
    bool IsFinished() const noexcept;

    // wait for this to become finished
    // should only be called from other context
    [[nodiscard]] FutureStatus WaitUntil(Deadline) const noexcept;

    TaskProcessor& GetTaskProcessor() { return task_processor_; }
    void DoStep();

    // normally non-blocking, causes wakeup
    void RequestCancel(TaskCancellationReason);

    TaskCancellationReason CancellationReason() const noexcept { return cancellation_reason_; }

    bool IsCancelRequested() const noexcept { return cancellation_reason_ != TaskCancellationReason::kNone; }

    bool IsCancellable() const noexcept;
    // returns previous value
    bool SetCancellable(bool);

    bool ShouldCancel() const noexcept { return IsCancelRequested() && IsCancellable(); }

    void SetBackground(bool);
    bool IsBackground() const noexcept { return is_background_; };

    static constexpr std::size_t kUnsetThreadIndex = static_cast<std::size_t>(-1);

    void SetThreadPinning(std::size_t thread_index) noexcept {
        UASSERT(thread_index_ == kUnsetThreadIndex);
        UASSERT(thread_index != kUnsetThreadIndex);
        thread_index_ = thread_index;
    }

    std::size_t GetThreadPinning() const noexcept { return thread_index_; }

    // causes this to yield and wait for wakeup
    // must only be called from this context
    // "spurious wakeups" may be caused by wakeup queueing
    WakeupSource Sleep(WaitStrategy& wait_strategy, Deadline deadline);

    // sleep epoch increments after each wakeup
    Epoch GetEpoch() const noexcept;

    // Awaiter's context. Effectively returns the same value as GetEpoch()
    std::uintptr_t GetAwaiterContext() const noexcept;

    // causes this to return from the nearest sleep
    // i.e. wakeup is queued if task is running
    // normally non-blocking, except corner cases in TaskProcessor::Schedule()
    void Wakeup(WakeupSource, Epoch epoch) noexcept;
    void Wakeup(WakeupSource, NoEpoch) noexcept;
    void Wakeup(WakeupSource, std::uintptr_t context) noexcept;

    static void CoroFunc(TaskPipe& task_pipe);

    // C++ ABI support, not to be used by anyone
    EhGlobals* GetEhGlobals() { return &eh_globals_; }

    TaskId GetTaskId() const { return reinterpret_cast<TaskId>(this); }

    std::chrono::steady_clock::time_point GetQueueWaitTimepoint() const { return task_queue_wait_timepoint_; }

    void SetQueueWaitTimepoint(std::chrono::steady_clock::time_point tp) { task_queue_wait_timepoint_ = tp; }

    void SetCancelDeadline(Deadline deadline);
    Deadline GetCancelDeadline() const noexcept { return cancel_deadline_; }

    bool HasLocalStorage() const noexcept;
    task_local::Storage& GetLocalStorage() noexcept;

    // ContextAccessor implementation
    bool IsReady() const noexcept override;
    void TryAppendAwaiter(boost::intrusive_ptr<Awaiter>& awaiter, std::uintptr_t context) override;
    void RemoveAwaiter(Awaiter& awaiter, std::uintptr_t context) noexcept override;
    std::exception_ptr GetErrorResult() const noexcept override;

    std::size_t DecrementFetchSharedTaskUsages() noexcept;
    std::size_t IncrementFetchSharedTaskUsages() noexcept;
    void ResetPayload() noexcept;

    CountedCoroutinePtr& GetCoroutinePtr() noexcept;

    bool WasStartedAsCritical() const;

    utils::StringLiteral GetActorType() const override;

private:
    class YieldReasonGuard;
    class LocalStorageGuard;
    class ProfilerExecutionGuard;

    static constexpr uint64_t kMagic = 0x6b73615453755459ULL;  // "YTuSTask"

    void ArmDeadlineTimer(Deadline deadline, Epoch sleep_epoch);
    void ArmCancellationTimer();

    static WakeupSource GetPrimaryWakeupSource(SleepState::Flags sleep_flags);

    void SetState(Task::State) noexcept;

    void Schedule() noexcept;
    static bool ShouldSchedule(SleepState::Flags flags, WakeupSource source) noexcept;

    void ProfilerStartExecution() noexcept;
    void ProfilerStopExecution() noexcept;

    void TraceStateTransition(Task::State state) noexcept;

    friend void intrusive_ptr_release(Awaiter* awaiter) noexcept;  // NOLINT(readability-identifier-naming)

    // Called from intrusive_ptr_release. Should delete the instance
    void Destroy() noexcept;

    const uint64_t magic_{kMagic};
    TaskProcessor& task_processor_;
    TaskCounter::Token task_counter_token_;
    const bool is_critical_;
    bool is_cancellable_{true};
    bool is_background_{false};
    bool within_sleep_{false};
    EhGlobals eh_globals_;

    utils::impl::WrappedCallBase* payload_;
    std::exception_ptr exception_;

    std::atomic<Task::State> state_{Task::State::kNew};
    std::atomic<DetachedTasksSyncBlock::Token*> detached_token_{nullptr};
    std::atomic<TaskCancellationReason> cancellation_reason_{TaskCancellationReason::kNone};
    FastPimplGenericWaitList finish_awaiters_;

    ContextTimer deadline_timer_;
    engine::Deadline cancel_deadline_;

    // {} if not defined
    std::chrono::steady_clock::time_point task_queue_wait_timepoint_;
    std::chrono::steady_clock::time_point execute_started_;
    std::chrono::steady_clock::time_point last_state_change_timepoint_;

    std::size_t trace_csw_left_;

    AtomicSleepState sleep_state_{SleepState{SleepFlags::kSleeping, Epoch{0}}};
    WakeupSource wakeup_source_{WakeupSource::kNone};

    CountedCoroutinePtr coro_;
    TaskPipe* task_pipe_{nullptr};
    YieldReason yield_reason_{YieldReason::kNone};

    std::optional<task_local::Storage> local_storage_{};

    // refcounter for task abandoning (cancellation) in engine::SharedTask
    std::atomic<std::size_t> shared_task_usages_{1};

    // for thread pinning task processors
    std::size_t thread_index_{kUnsetThreadIndex};
};

bool HasWaitSucceeded(TaskContext::WakeupSource) noexcept;

}  // namespace impl

namespace current_task {

engine::impl::TaskContext& GetCurrentTaskContext() noexcept;
engine::impl::TaskContext* GetCurrentTaskContextUnchecked() noexcept;

}  // namespace current_task
}  // namespace engine

USERVER_NAMESPACE_END
