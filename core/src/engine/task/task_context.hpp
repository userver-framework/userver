#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <optional>

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
#include <userver/engine/impl/task_local_storage.hpp>
#include <userver/engine/impl/wait_list_fwd.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/flags.hpp>
#include <userver/utils/impl/intrusive_ref_counter_one.hpp>
#include <userver/utils/impl/wrapped_call_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {
namespace impl {

class TaskContextHolder;

[[noreturn]] void ReportDeadlock();

// Data owned by TaskContext but driven by ProfilerExecutionPlugin. Tracks when
// the current execution slice started so the plugin can warn about tasks that
// run for too long without a context switch.
struct ProfilerExecutionData final {
    std::chrono::steady_clock::time_point execute_started{};
};

// Data owned by TaskContext but driven by TraceStateTransitionPlugin. Tracks the
// remaining number of context switches to trace and the timepoint of the last
// traced state change.
struct TraceStateTransitionData final {
    std::size_t trace_csw_left{};
    std::chrono::steady_clock::time_point last_state_change_timepoint{};
};

struct TaskContextDeleter {
    void operator()(TaskContext* task_context) noexcept;
};

// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class TaskContext final
    : public utils::impl::IntrusiveRefCounterOne<TaskContext, TaskContextDeleter>,
      public AwaitableBase,
      public Awaiter,
      public deadlock_detector::Actor {
public:
    using TaskPipe = coro::Pool::TaskPipe;
    using TaskId = uint64_t;

    enum class YieldReason { kNone, kTaskWaiting, kTaskComplete };

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

    // The terminal state the task is finishing with (kCompleted or kCancelled).
    // Only meaningful once the task body has finished, e.g. from HookTaskStop.
    Task::State GetPendingFinalState() const noexcept { return pending_final_state_; }

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

    TaskProcessor& GetTaskProcessor() noexcept { return task_processor_; }
    void DoStep(boost::intrusive_ptr<TaskContext>&& self);

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
    WakeupSource Sleep(WeakAwaitable& awaitable, Deadline deadline);

    // sleep epoch increments before each sleep attempt
    Epoch GetEpoch() const noexcept;

    // Awaiter's context. Effectively returns the same value as GetEpoch()
    std::uintptr_t GetAwaiterContext() const noexcept;

    // causes `self` to return from the nearest sleep
    // i.e. wakeup is queued if task is running
    // normally non-blocking, except corner cases in TaskProcessor::Schedule()
    static void Wakeup(boost::intrusive_ptr<TaskContext>&& self, WakeupSource, Epoch epoch) noexcept;
    static void Wakeup(boost::intrusive_ptr<TaskContext>&& self, WakeupSource, NoEpoch) noexcept;

    AwaiterPtr AsAwaiterPtr() noexcept;

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

    // Awaitable implementation
    bool IsReady() const noexcept override;
    void TryAppendAwaiter(AwaiterPtr& awaiter, std::uintptr_t context) override;
    AwaiterPtr RemoveAwaiter(Awaiter& awaiter, std::uintptr_t context) noexcept override;
    std::exception_ptr GetErrorResult() const noexcept override;

    std::size_t DecrementFetchSharedTaskUsages() noexcept;
    std::size_t IncrementFetchSharedTaskUsages() noexcept;
    void ResetPayload() noexcept;

    CountedCoroutinePtr& GetCoroutinePtr() noexcept;

    bool WasStartedAsCritical() const;

    utils::StringLiteral GetActorType() const override;

    // Accessors for the data driven by plugins.
    ProfilerExecutionData& GetProfilerExecutionData() noexcept { return profiler_execution_data_; }
    TraceStateTransitionData& GetTraceStateTransitionData() noexcept { return trace_state_transition_data_; }

private:
    class YieldReasonGuard;
    class LocalStorageGuard;
    class TaskStartStopHookGuard;

    static constexpr uint64_t kMagic = 0x6b73615453755459ULL;  // "YTuSTask"

    void ArmDeadlineTimer(Deadline deadline, Epoch sleep_epoch);
    void ArmCancellationTimer();

    static WakeupSource GetPrimaryWakeupSource(SleepState::Flags sleep_flags);

    void SetState(Task::State) noexcept;

    // Sets the wakeup `source` flag in sleep_state_ if it still refers to `epoch`.
    // Returns the previous SleepState, or std::nullopt if the epoch has already changed.
    std::optional<SleepState> SetSleepStateWakeupSourceForEpoch(WakeupSource source, Epoch epoch) noexcept;

    static void Schedule(boost::intrusive_ptr<TaskContext>&& self) noexcept;
    static bool ShouldSchedule(SleepState::Flags flags, WakeupSource source) noexcept;

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
    std::atomic<TaskCancellationReason> cancellation_reason_{TaskCancellationReason::kNone};
    FastPimplGenericWaitList finish_awaiters_;

    ContextTimer deadline_timer_;
    engine::Deadline cancel_deadline_;

    // {} if not defined
    std::chrono::steady_clock::time_point task_queue_wait_timepoint_;

    // Storage driven by the trace/profiler plugins.
    ProfilerExecutionData profiler_execution_data_;
    TraceStateTransitionData trace_state_transition_data_;

    AtomicSleepState sleep_state_{SleepState{SleepFlags::kSleeping, Epoch{0}}};

    CountedCoroutinePtr coro_;
    TaskPipe* task_pipe_{nullptr};
    YieldReason yield_reason_{YieldReason::kNone};
    Task::State pending_final_state_{Task::State::kCompleted};

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
