#include "task_context.hpp"

#include <exception>
#include <limits>
#include <utility>

#include <fmt/format.h>
#include <boost/exception/diagnostic_information.hpp>

#include <engine/coro/pool.hpp>
#include <engine/coro/stack_usage_monitor.hpp>
#include <userver/compiler/impl/tls.hpp>
#include <userver/compiler/impl/tsan.hpp>
#include <userver/compiler/thread_local.hpp>
#include <userver/engine/exception.hpp>
#include <userver/engine/impl/task_context_factory.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/stacktrace_cache.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/underlying_value.hpp>

#include <engine/deadlock_detector.hpp>
#include <engine/ev/thread_pool.hpp>
#include <engine/impl/future_utils.hpp>
#include <engine/impl/generic_wait_list.hpp>
#include <engine/task/coro_unwinder.hpp>
#include <engine/task/cxxabi_eh_globals.hpp>
#include <engine/task/task_processor.hpp>

#include <gdb_autogen/cmd/utask/cmd.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace current_task {
namespace {

compiler::ThreadLocal current_task_context_ptr = []() -> engine::impl::TaskContext* { return nullptr; };

void SetCurrentTaskContext(engine::impl::TaskContext* context) {
    auto local_task_context_ptr = current_task_context_ptr.Use();
    UASSERT(!*local_task_context_ptr || !context);
    *local_task_context_ptr = context;
}

}  // namespace

engine::impl::TaskContext& GetCurrentTaskContext() noexcept {
    auto current_task_context = current_task_context_ptr.Use();
    if (!*current_task_context) {
        // AbortWithStacktrace MUST be a separate function! Putting the body of this
        // function into GetCurrentTaskContext() clobbers too many registers and
        // compiler decides to use stack memory in GetCurrentTaskContext(). This
        // leads to slowdown of GetCurrentTaskContext(). In particular Mutex::lock()
        // slows down on ~25%.
        utils::AbortWithStacktrace(
            "current_task::GetCurrentTaskContext() has been called "
            "outside of coroutine context"
        );
    }
    return **current_task_context;
}

engine::impl::TaskContext* GetCurrentTaskContextUnchecked() noexcept {
    auto current_task_context = current_task_context_ptr.Use();
    return *current_task_context;
}

}  // namespace current_task

namespace impl {

[[noreturn]] void ReportDeadlock() { UINVARIANT(false, "Coroutine attempted to wait for itself"); }

namespace {

auto ReadableTaskId(const TaskContext* task) noexcept { return logging::HexShort(task ? task->GetTaskId() : 0); }

class CurrentTaskScope final {
public:
    explicit CurrentTaskScope(TaskContext& context, EhGlobals& eh_store)
        : eh_store_(eh_store)
    {
        current_task::SetCurrentTaskContext(&context);
        ExchangeEhGlobals(eh_store_);
    }

    ~CurrentTaskScope() {
        ExchangeEhGlobals(eh_store_);
        current_task::SetCurrentTaskContext(nullptr);
    }

private:
    EhGlobals& eh_store_;
};

constexpr SleepState MakeNextEpochSleepState(SleepState current) {
    return {SleepFlags::kNone, Epoch{utils::UnderlyingValue(current.epoch) + 1}};
}

}  // namespace

void TaskContextDeleter::operator()(TaskContext* task_context) noexcept {
    UASSERT(task_context);
    task_context->ResetPayload();
    std::destroy_at(task_context);
    DeleteFusedTaskContext(reinterpret_cast<std::byte*>(task_context));
}

TaskContext::TaskContext(
    TaskProcessor& task_processor,
    Task::Importance importance,
    Task::WaitMode wait_type,
    Deadline deadline,
    TaskInheritedVariablePriority inherited_variables_priority,
    utils::impl::WrappedCallBase& payload
)
    : Awaiter(Awaiter::StaticType::kTaskContext),
      task_processor_(task_processor),
      task_counter_token_(task_processor_.GetTaskCounter()),
      is_critical_(importance == Task::Importance::kCritical),
      payload_(&payload),
      finish_awaiters_(wait_type),
      cancel_deadline_(deadline)
{
    UASSERT(payload_);

    // Inherit the task-inherited variables from the parent synchronously, while they still exist.
    local_storage_.emplace();
    if (auto* current_task_context = current_task::GetCurrentTaskContextUnchecked()) {
        local_storage_->InheritFrom(*current_task_context->local_storage_, inherited_variables_priority);
    }

    task_processor_.HookTaskCreate(*this);
    LOG_TRACE()
        << "task with task_id=" << ReadableTaskId(current_task::GetCurrentTaskContextUnchecked())
        << " created task with task_id=" << ReadableTaskId(this) << logging::LogExtra::Stacktrace();
}

TaskContext::~TaskContext() noexcept {
    task_processor_.HookTaskDestroy(*this);
    LOG_TRACE() << "Task with task_id=" << ReadableTaskId(this) << " stopped" << logging::LogExtra::Stacktrace();
    UASSERT(magic_ == kMagic);

    UASSERT(state_ == Task::State::kNew || IsFinished());

    UASSERT(payload_ == nullptr);
}

utils::impl::WrappedCallBase& TaskContext::GetPayload() noexcept {
    UASSERT(state_.load(std::memory_order_relaxed) == Task::State::kCompleted);
    UASSERT(payload_);
    return *payload_;
}

bool TaskContext::IsCurrent() const noexcept { return this == current_task::GetCurrentTaskContextUnchecked(); }

bool TaskContext::IsCritical() const {
    // running tasks must not be susceptible to overload
    // e.g. we might need to run coroutine to cancel it
    return WasStartedAsCritical() || coro_;
}

bool TaskContext::IsSharedWaitAllowed() const { return finish_awaiters_->IsShared(); }

bool TaskContext::IsFinished() const noexcept { return finish_awaiters_->IsSignaled(); }

FutureStatus TaskContext::WaitUntil(Deadline deadline) const noexcept {
    // try to avoid ctx switch if possible
    static_assert(noexcept(IsFinished()));
    if (IsFinished()) {
        return FutureStatus::kReady;
    }

    static_assert(noexcept(current_task::GetCurrentTaskContext()));
    auto& current = current_task::GetCurrentTaskContext();

    std::optional<engine::deadlock_detector::WaitScope> scope;
    if (!deadline.IsReachable()) {
        scope.emplace(*this);
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    auto& target = const_cast<TaskContext&>(*this);

    try {
        const auto wakeup_source = current.Sleep(target, deadline);
        return ToFutureStatus(wakeup_source);
    } catch (...) {
        // We cannot just refuse to wait because of the lifetime guarantees for tasks and their data.
        utils::AbortWithStacktrace(
            "Unexpected exception from Sleep: " + boost::current_exception_diagnostic_information()
        );
    }
}

void TaskContext::DoStep(boost::intrusive_ptr<TaskContext>&& self) {
    UASSERT(self == this);

    if (IsFinished()) {
        return;
    }

    if (!coro_) {
        try {
            coro_ = task_processor_.GetCoroutine();
        } catch (...) {
            // Seems we're out of memory
            ResetPayload();
            cancellation_reason_ = TaskCancellationReason::kOOM;
            GetTaskProcessor().GetTaskCounter().AccountTaskCancel();
            SetState(TaskBase::State::kCancelled);
            finish_awaiters_->SetSignalAndNotifyAll();
            throw;
        }

        ArmCancellationTimer();
    }

    // eh_globals is replaced in task scope, we must proxy the exception
    std::exception_ptr uncaught;
    {
        const CurrentTaskScope current_task_scope(*this, eh_globals_);
        try {
            SetState(Task::State::kRunning);
            auto& coro_ref = *coro_;
            coro_ref(this);
        } catch (...) {
            uncaught = std::current_exception();
        }
    }

    if (uncaught) {
        std::rethrow_exception(uncaught);
    }

    switch (yield_reason_) {
        case YieldReason::kTaskComplete: {
            std::move(coro_).ReturnToPool();
            const auto new_state = pending_final_state_;
            if (cancellation_reason_.load(std::memory_order_relaxed) != TaskCancellationReason::kNone) {
                GetTaskProcessor().GetTaskCounter().AccountTaskCancel();
            }
            SetState(new_state);
            deadline_timer_.Finalize();
            finish_awaiters_->SetSignalAndNotifyAll();
        } break;

        case YieldReason::kTaskWaiting:
            SetState(Task::State::kSuspended);
            {
                SleepState::Flags new_flags = SleepFlags::kSleeping;
                if (!IsCancellable()) {
                    new_flags |= SleepFlags::kNonCancellable;
                }

                // Synchronization point for relaxed SetState()
                auto prev_sleep_state = sleep_state_.FetchOrFlags<std::memory_order_seq_cst>(new_flags);

                UASSERT(!(prev_sleep_state.flags & SleepFlags::kSleeping));
                if (new_flags & SleepFlags::kNonCancellable) {
                    prev_sleep_state.flags.Clear({SleepFlags::kWakeupByCancelRequest, SleepFlags::kNonCancellable});
                }

                if (prev_sleep_state.flags) {
                    Schedule(std::move(self));
                } else if (!(new_flags & SleepFlags::kNonCancellable) &&
                           cancellation_reason_.load() != TaskCancellationReason::kNone) [[unlikely]]
                {
                    // cancellation_reason_ is the source of truth. The edge-triggered cancellation in sleep_state_
                    // may have been erased by Sleep() while advancing the epoch.
                    //
                    // If RequestCancel() already did its seq_cst sleep_state_ wakeup before FetchOrFlags() above,
                    // then this seq_cst load observes its prior cancellation_reason_ write.
                    // Otherwise RequestCancel() runs wakeup after FetchOrFlags(), observes kSleeping,
                    // and schedules the task itself.
                    Wakeup(std::move(self), WakeupSource::kCancelRequest, prev_sleep_state.epoch);
                }
            }
            break;

        case YieldReason::kNone:
            UINVARIANT(false, "invalid yield reason");
    }
}

void TaskContext::RequestCancel(TaskCancellationReason reason) {
    auto expected = TaskCancellationReason::kNone;
    if (cancellation_reason_.compare_exchange_strong(expected, reason)) {
        LOG_TRACE()
            << "task with task_id=" << ReadableTaskId(current_task::GetCurrentTaskContextUnchecked())
            << " cancelled task with task_id=" << ReadableTaskId(this) << logging::LogExtra::Stacktrace();
        Wakeup(boost::intrusive_ptr<TaskContext>{this}, WakeupSource::kCancelRequest, NoEpoch{});
    }
}

bool TaskContext::IsCancellable() const noexcept { return is_cancellable_; }

bool TaskContext::SetCancellable(bool value) {
    UASSERT(IsCurrent());
    UASSERT(state_ == Task::State::kRunning);

    return std::exchange(is_cancellable_, value);
}

void TaskContext::SetBackground(bool is_background) {
    UASSERT(IsCurrent());
    UASSERT(state_ == Task::State::kRunning);
    is_background_ = is_background;
}

TaskContext::WakeupSource TaskContext::Sleep(WeakAwaitable& awaitable, Deadline deadline) {
    UASSERT(IsCurrent());
    UASSERT(state_ == Task::State::kRunning);
    UASSERT_MSG(
        compiler::impl::AreCoroutineSwitchesAllowed(),
        "Coroutine context switches are forbidden in the current scope, "
        "which is likely working with thread-local variables"
    );

    UASSERT_MSG(!std::exchange(within_sleep_, true), "Recursion in Sleep detected");
    const utils::FastScopeGuard guard{[this]() noexcept {
        UASSERT_MSG(std::exchange(within_sleep_, false), "within_sleep_ should report being in Sleep");
    }};

    const auto new_sleep_state = MakeNextEpochSleepState(sleep_state_.Load<std::memory_order_relaxed>());
    sleep_state_.Store<std::memory_order_relaxed>(new_sleep_state);

    // Fast path for already visible cancellation; missed cancellations are recovered in DoStep.
    if (ShouldCancel()) {
        return TaskContext::WakeupSource::kCancelRequest;
    }

    const auto sleep_epoch = new_sleep_state.epoch;
    const auto awaiter_context = static_cast<std::uintptr_t>(sleep_epoch);

    auto self = AsAwaiterPtr();
    awaitable.TryAppendAwaiter(self, awaiter_context);
    if (self != nullptr) {
        return WakeupSource::kNotify;
    }

    const bool has_deadline = deadline.IsReachable() && (!IsCancellable() || deadline < cancel_deadline_);
    if (has_deadline) {
        ArmDeadlineTimer(deadline, sleep_epoch);
    }

    yield_reason_ = YieldReason::kTaskWaiting;
    UASSERT(task_pipe_);

    GetTaskProcessor().HookBeforeSleep(*this);

    auto& task_pipe_ref = *task_pipe_;
    [[maybe_unused]] TaskContext* context = task_pipe_ref().get();

    GetTaskProcessor().HookAfterWakeup(*this);

    UASSERT(context == this);
    UASSERT(state_ == Task::State::kRunning);

    if (has_deadline) {
        ArmCancellationTimer();
    }
    awaitable.RemoveAwaiter(*this, awaiter_context);

    const auto sleep_flags = sleep_state_.Load<std::memory_order_acquire>().flags;
    return GetPrimaryWakeupSource(sleep_flags);
}

void TaskContext::ArmDeadlineTimer(Deadline deadline, Epoch sleep_epoch) {
    UASSERT(deadline.IsReachable());
    if (deadline_timer_.WasStarted()) {
        deadline_timer_.RestartWakeup(deadline, sleep_epoch);
    } else {
        deadline_timer_.StartWakeup(
            boost::intrusive_ptr{this},
            task_processor_.EventThreadPool().NextTimerThread(),
            deadline,
            sleep_epoch
        );
    }
}

void TaskContext::ArmCancellationTimer() {
    if (!cancel_deadline_.IsReachable()) {
        return;
    }

    if (deadline_timer_.WasStarted()) {
        deadline_timer_.RestartCancel(cancel_deadline_);
    } else {
        deadline_timer_.StartCancel(
            boost::intrusive_ptr{this},
            task_processor_.EventThreadPool().NextTimerThread(),
            cancel_deadline_
        );
    }
}

bool TaskContext::ShouldSchedule(SleepState::Flags prev_flags, WakeupSource source) noexcept {
    /* ShouldSchedule() returns true only for the first Wakeup(). All Wakeup()s
     * are serialized due to seq_cst modifications of sleep_state_.
     */

    if (!(prev_flags & SleepFlags::kSleeping)) {
        return false;
    }

    if (source == WakeupSource::kCancelRequest) {
        /* Don't wakeup if:
         * 1) kNonCancellable
         * 2) Other WakeupSource is already triggered
         */
        return prev_flags == SleepFlags::kSleeping;
    } else if (source == WakeupSource::kBootstrap) {
        return true;
    } else {
        if (prev_flags & SleepFlags::kNonCancellable) {
            /* If there was a cancellation request, but cancellation is blocked,
             * ignore it - we're the first to Schedule().
             */
            prev_flags.Clear({SleepFlags::kNonCancellable, SleepFlags::kWakeupByCancelRequest});
        }

        /* Don't wakeup if:
         * 1) kNonCancellable and zero or more kCancelRequest triggered
         * 2) !kNonCancellable and any other WakeupSource is triggered
         */

        // We're the first to wakeup the baby
        return prev_flags == SleepFlags::kSleeping;
    }
}

Epoch TaskContext::GetEpoch() const noexcept { return sleep_state_.Load<std::memory_order_acquire>().epoch; }

std::uintptr_t TaskContext::GetAwaiterContext() const noexcept { return static_cast<std::uintptr_t>(GetEpoch()); }

std::optional<SleepState> TaskContext::SetSleepStateWakeupSourceForEpoch(WakeupSource source, Epoch epoch) noexcept {
    auto prev_sleep_state = sleep_state_.Load<std::memory_order_relaxed>();

    while (true) {
        if (prev_sleep_state.epoch != epoch) {
            // Epoch changed, wakeup is for some previous sleep
            return std::nullopt;
        }

        auto new_sleep_state = prev_sleep_state;
        new_sleep_state.flags |= static_cast<SleepFlags>(source);

        // 1) thread1: calls context.GetQueueWaitTimepoint()
        // 2) thread1: starts waiting for some task in thread2
        // 3) thread2: wakes up the thread1-coroutine via TaskContext::Wakeup
        // 4) thread2: TaskContext::Wakeup in Schedule() calls context.SetQueueWaitTimepoint()
        //
        // Without std::memory_order_seq_cst in this function TSAN reports a data race at step 4) on
        // a ServerMinimalComponentList.Basic test. Run the test under TSAN multiple times if planning to change.
        if (sleep_state_.CompareExchangeWeak<
                std::memory_order_seq_cst,
                std::memory_order_relaxed>(prev_sleep_state, new_sleep_state))
        {
            return prev_sleep_state;
        }
    }
}

void TaskContext::Wakeup(boost::intrusive_ptr<TaskContext>&& self, WakeupSource source, Epoch epoch) noexcept {
    UASSERT(self);
    if (self->IsFinished()) {
        return;
    }

    const auto prev_sleep_state = self->SetSleepStateWakeupSourceForEpoch(source, epoch);
    if (!prev_sleep_state.has_value()) {
        return;
    }

    if (ShouldSchedule(prev_sleep_state->flags, source)) {
        Schedule(std::move(self));
    }
}

void TaskContext::Wakeup(boost::intrusive_ptr<TaskContext>&& self, WakeupSource source, NoEpoch) noexcept {
    UASSERT(self);
    UASSERT(source != WakeupSource::kDeadlineTimer);
    UASSERT(source != WakeupSource::kBootstrap);

    if (self->IsFinished()) {
        return;
    }

    // Set flag regardless of kSleeping - missing kSleeping usually means one of the following:
    // * the task is somewhere between Sleep() and setting kSleeping in DoStep().
    // * the task is already awaken, but DisableWakeups() is not yet finished (and not all timers/watchers are stopped).
    const auto
        prev_sleep_state = self->sleep_state_.FetchOrFlags<std::memory_order_seq_cst>(static_cast<SleepFlags>(source));
    if (ShouldSchedule(prev_sleep_state.flags, source)) {
        Schedule(std::move(self));
    }
}

AwaiterPtr TaskContext::AsAwaiterPtr() noexcept {
    intrusive_ptr_add_ref(this);
    return AwaiterPtr{this};
}

class TaskContext::YieldReasonGuard {
public:
    explicit YieldReasonGuard(TaskContext& context) noexcept : context_(context) {}

    // The terminal state (completed vs cancelled) is conveyed via pending_final_state_.
    ~YieldReasonGuard() noexcept { context_.yield_reason_ = YieldReason::kTaskComplete; }

private:
    TaskContext& context_;
};

class TaskContext::LocalStorageGuard {
public:
    explicit LocalStorageGuard(TaskContext& context)
        : context_(context)
    {
        UASSERT(context_.local_storage_.has_value());
    }

    ~LocalStorageGuard() {
        // Destroy the variables while the storage is still alive and reachable
        // through GetCurrentStorage: the destructors may sleep, letting
        // arbitrary engine code (e.g. plugin hooks) access task-locals.
        // Calling this from ~Storage during optional::reset would make that
        // access UB.
        context_.local_storage_->DestroyVariables();
        context_.local_storage_.reset();
    }

private:
    TaskContext& context_;
};

class TaskContext::TaskStartStopHookGuard {
public:
    explicit TaskStartStopHookGuard(TaskContext& context) noexcept : context_(context) {
        static_assert(noexcept(context_.GetTaskProcessor().HookTaskStart(context_)));
        context_.GetTaskProcessor().HookTaskStart(context_);
    }

    ~TaskStartStopHookGuard() {
        static_assert(noexcept(context_.GetTaskProcessor().HookTaskStop(context_)));
        context_.GetTaskProcessor().HookTaskStop(context_);
    }

private:
    TaskContext& context_;
};

void TaskContext::CoroFunc(TaskPipe& task_pipe) {
    for (TaskContext* context : task_pipe) {
        // `context` is accessed in gdb, do not rename
        UASSERT(context);
        context->task_pipe_ = &task_pipe;

        {
            // Set yield_reason_ outside ~LocalStorageGuard as Sleep in dtors would otherwise clobber it.
            const YieldReasonGuard yield_reason_guard(*context);
            // Destroy contents of LocalStorage in the coroutine as dtors may want to schedule.
            const LocalStorageGuard local_storage_guard(*context);
            // Drives the task start/stop hooks (trace/profiler plugins) with task-local storage alive.
            const TaskStartStopHookGuard task_start_stop_hook_guard(*context);

            // We only let tasks ran with CriticalAsync enter function body, others
            // get terminated ASAP.
            if (context->IsCancelRequested() && !context->WasStartedAsCritical()) {
                context->SetCancellable(false);
                // It is important to destroy payload here as someone may want
                // to synchronize in its dtor (e.g. lambda closure).
                context->ResetPayload();
                context->pending_final_state_ = Task::State::kCancelled;
            } else {
                try {
                    context->payload_->Perform();
                    // We store an exception in the context to be able to handle
                    // Awaitable::GetErrorResult() even when the owning
                    // task is destroyed (and payload_ is reset to nullptr).
                    context->exception_ = context->payload_->GetException();
                    UASSERT(context->pending_final_state_ == Task::State::kCompleted);
                } catch (const CoroUnwinder&) {
                    context->pending_final_state_ = Task::State::kCancelled;
                } catch (...) {
                    utils::AbortWithStacktrace(
                        "An exception that is not derived from std::exception has been "
                        "thrown: " +
                        boost::current_exception_diagnostic_information() +
                        " Such exceptions are not supported by userver."
                    );
                }
            }
        }

        context->task_pipe_ = nullptr;
    }
}

void TaskContext::SetCancelDeadline(Deadline deadline) {
    UASSERT(IsCurrent());
    UASSERT(state_ == Task::State::kRunning);
    cancel_deadline_ = deadline;
    ArmCancellationTimer();
}

bool TaskContext::HasLocalStorage() const noexcept { return local_storage_.has_value(); }

task_local::Storage& TaskContext::GetLocalStorage() noexcept {
    UASSERT(local_storage_);
    return *local_storage_;
}

bool TaskContext::IsReady() const noexcept { return IsFinished(); }

void TaskContext::TryAppendAwaiter(AwaiterPtr& awaiter, std::uintptr_t context) {
    if (awaiter.get() == static_cast<Awaiter*>(this)) {
        ReportDeadlock();
    }
    finish_awaiters_->GetSignalOrAppend(awaiter, context);
}

AwaiterPtr TaskContext::RemoveAwaiter(Awaiter& awaiter, std::uintptr_t context) noexcept {
    return finish_awaiters_->Remove(awaiter, context);
}

std::exception_ptr TaskContext::GetErrorResult() const noexcept {
    UASSERT(IsFinished());
    if (state_.load(std::memory_order_acquire) != Task::State::kCompleted) {
        return std::make_exception_ptr(TaskCancelledException(CancellationReason()));
    }
    if (exception_) {
        return exception_;
    }
    return {};
}

std::size_t TaskContext::DecrementFetchSharedTaskUsages() noexcept { return --shared_task_usages_; }

std::size_t TaskContext::IncrementFetchSharedTaskUsages() noexcept { return ++shared_task_usages_; }

TaskContext::WakeupSource TaskContext::GetPrimaryWakeupSource(SleepState::Flags sleep_flags) {
    static constexpr std::pair<SleepState::Flags, WakeupSource> l[] = {
        {SleepFlags::kWakeupByWaitList, WakeupSource::kNotify},
        {SleepFlags::kWakeupByDeadlineTimer, WakeupSource::kDeadlineTimer},
        {SleepFlags::kWakeupByBootstrap, WakeupSource::kBootstrap},
    };
    for (auto it : l) {
        if (sleep_flags & it.first) {
            return it.second;
        }
    }

    if ((sleep_flags & SleepFlags::kWakeupByCancelRequest) && !(sleep_flags & SleepFlags::kNonCancellable)) {
        return WakeupSource::kCancelRequest;
    }

    UINVARIANT(false, fmt::format("Cannot find valid wakeup source for {}", sleep_flags.GetValue()));
}

bool TaskContext::WasStartedAsCritical() const { return is_critical_; }

void TaskContext::SetState(Task::State new_state) noexcept {
    // 'release', because if someone detects kCompleted or kCancelled by running
    // in a loop, they should acquire the task's results.
    state_.store(new_state, std::memory_order_release);
}

void TaskContext::Schedule(boost::intrusive_ptr<TaskContext>&& self) noexcept {
    UASSERT(self);
    UASSERT(self->state_ != Task::State::kQueued);
    self->SetState(Task::State::kQueued);
    auto& task_processor = self->task_processor_;
    try {
        task_processor.Schedule(std::move(self));
    } catch (...) {
        // We cannot just refuse to run the task because of the lifetime guarantees for tasks and their data.
        utils::AbortWithStacktrace(
            "Unexpected exception from Schedule: " + boost::current_exception_diagnostic_information()
        );
    }
    // NOTE: may be executed at this point
}

void TaskContext::ResetPayload() noexcept {
    if (!payload_) {
        return;
    }

    std::destroy_at(std::exchange(payload_, nullptr));
}

CountedCoroutinePtr& TaskContext::GetCoroutinePtr() noexcept { return coro_; }

utils::StringLiteral TaskContext::GetActorType() const { return "Task"; }

bool HasWaitSucceeded(TaskContext::WakeupSource wakeup_source) noexcept {
    // Typical synchronization primitives sleep in a WaitList until woken up
    // (which is counted as a success), or they can sometimes wake themselves up
    // using kNotify.
    switch (wakeup_source) {
        case TaskContext::WakeupSource::kNotify:
            return true;
        case TaskContext::WakeupSource::kDeadlineTimer:
        case TaskContext::WakeupSource::kCancelRequest:
            return false;
        case TaskContext::WakeupSource::kNone:
        case TaskContext::WakeupSource::kBootstrap:
            UASSERT(false);
    }

    // Assume that bugs with an unexpected WakeupSource don't reach production.
    return false;
}

}  // namespace impl
}  // namespace engine

#include "cxxabi_eh_globals.inc"

USERVER_NAMESPACE_END
