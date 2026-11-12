#include <ugrpc/impl/event_engine/timer_manager.hpp>

#if defined(ARCADIA_ROOT) && defined(__linux__)

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <utility>

#include <userver/engine/deadline.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fast_scope_guard.hpp>

#include <engine/ev/thread_pool.hpp>

#include <src/core/lib/iomgr/exec_ctx.h>

#include <ugrpc/impl/event_engine/executor.hpp>
#include <ugrpc/impl/event_engine/timer_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

namespace {

constexpr std::chrono::hours kMaxDeferDelay{1};

engine::Deadline GetDeferDeadline(event_engine::TimerClock::time_point deadline) {
    const auto max_defer = engine::Deadline::FromDuration(kMaxDeferDelay);
    return engine::Deadline::FromTimePoint(std::min(deadline, max_defer.GetTimePoint()));
}

void FlushDeferredPayload(engine::ev::ThreadPool& pool) {
    for (auto& control : pool.GetTimerThreads()) {
        control.RunInEvLoopSync([]() noexcept {});
    }
}

}  // namespace

TimerPool::TaskHandle TimerPool::TimerData::GetHandle() const {
    return {{reinterpret_cast<std::intptr_t>(this), expected}};
}

void TimerManager::OnTimer(struct ev_loop*, ev_timer* timer, int) {
    auto& timerd = *static_cast<TimerPool::TimerData*>(timer->data);
    UASSERT(timerd.thread_control->IsInEvThread());
    timerd.stopped = true;
    auto expected = timerd.expected;
    if (!timerd.revision.compare_exchange_strong(expected, timerd.expected + 1)) {
        return;
    }

    auto closure = std::exchange(timerd.closure, {});
    auto& executor = timerd.timer_pool->executor;
    Release(timerd);
    executor.Run(std::move(closure));
}

void TimerPool::TimerInitPayload::DoPerformAndRelease() {
    auto& timerd = timerd_;
    UASSERT(timerd.thread_control->IsInEvThread());
    timerd.timer_init_payload.reset();
    if (timerd.started.exchange(true)) {
        Release(timerd);
        return;
    }
    const auto after = event_engine::GetTimerDelay(timerd.deadline, event_engine::TimerClock::now());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_timer_set(&timerd.timer, after, 0.0);
    timerd.thread_control->Start(timerd.timer);
}

void TimerPool::TimerCancelPayload::DoPerformAndRelease() {
    auto& timerd = timerd_;
    UASSERT(timerd.thread_control->IsInEvThread());
    if (!timerd.stopped) {
        timerd.thread_control->Stop(timerd.timer);
    }
    timerd.timer_cancel_payload.reset();
    Release(timerd);
}

TimerManager::TimerManager(engine::ev::ThreadPool& ev_thread_pool, Executor& executor)
    : ev_thread_pool_(ev_thread_pool),
      timer_pool_(executor)
{
    UINVARIANT(ev_thread_pool_.GetSize() != 0, "Configure at least one EV thread for the gRPC TimerManager");
}

TimerManager::~TimerManager() { Shutdown(); }

bool TimerManager::TryStartOperation() noexcept {
    auto operations = operations_.load(std::memory_order_acquire);
    while ((operations & kStoppingBit) == 0) {
        UASSERT(operations < kStoppingBit - 1);
        if (operations_.compare_exchange_weak(
                operations,
                operations + 1,
                std::memory_order_acq_rel,
                std::memory_order_acquire
            ))
        {
            return true;
        }
    }
    return false;
}

void TimerManager::FinishOperation() noexcept {
    const auto previous = operations_.fetch_sub(1, std::memory_order_acq_rel);
    UASSERT((previous & ~kStoppingBit) != 0);
    if (previous == kStoppingBit + 1) {
        operations_drained_.Send();
    }
}

TimerManager::TaskHandle TimerManager::TimerInit(Duration when, AnyInvocable closure) {
    UASSERT(closure);
    if (when <= Duration::zero()) {
        timer_pool_.executor.Run(std::move(closure));
        return TaskHandle::kInvalid;
    }
    if (!TryStartOperation()) {
        Executor::RunInCallbackScope([&] { closure = {}; });
        return TaskHandle::kInvalid;
    }
    const utils::FastScopeGuard operation([this]() noexcept { FinishOperation(); });
    const auto deadline = event_engine::MakeTimerDeadline(event_engine::TimerClock::now(), when);
    auto& timerd = Acquire(timer_pool_);
    timerd.expected = timerd.revision.load();
    UASSERT(!timerd.closure);
    timerd.closure = std::move(closure);
    timerd.thread_control = &ev_thread_pool_.NextTimerThread();
    timerd.deadline = deadline;
    timerd.started.store(false);
    timerd.stopped = false;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_timer_init(&timerd.timer, &TimerManager::OnTimer, 0.0, 0.0);
    timerd.timer.data = &timerd;
    timerd.timer_init_payload.emplace(timerd);
    const auto handle = timerd.GetHandle();
    timerd.thread_control->RunPayloadInEvLoopDeferred(*timerd.timer_init_payload, GetDeferDeadline(deadline));
    return handle;
}

bool TimerManager::TimerCancel(TaskHandle handle) {
    if (handle == TaskHandle::kInvalid || !TryStartOperation()) {
        return false;
    }
    const utils::FastScopeGuard operation([this]() noexcept { FinishOperation(); });
    return CancelTimer(handle);
}

bool TimerManager::CancelTimer(TaskHandle handle) {
    auto& timerd = *reinterpret_cast<TimerPool::TimerData*>(handle.keys[0]);
    UASSERT_MSG(timerd.timer_pool == &timer_pool_, "TimerCancel requires a handle from this TimerManager");
    const std::intptr_t revision = handle.keys[1];
    auto expected = revision;
    if (!timerd.revision.compare_exchange_strong(expected, revision + 1)) {
        return false;
    }

    Executor::RunInCallbackScope([&] { timerd.closure = {}; });
    if (timerd.started.exchange(true)) {
        timerd.timer_cancel_payload.emplace(timerd);
        timerd.thread_control
            ->RunPayloadInEvLoopDeferred(*timerd.timer_cancel_payload, engine::Deadline::FromDuration(kMaxDeferDelay));
    }
    return true;
}

void TimerManager::Shutdown() {
    if (state_.load() == State::kStopped) {
        return;
    }
    UINVARIANT(
        !Executor::IsInCallback(),
        "Call TimerManager::Shutdown outside gRPC callbacks and their capture destructors"
    );
    UINVARIANT(engine::current_task::IsTaskProcessorThread(), "Call TimerManager::Shutdown from a coroutine");
    auto expected = State::kRunning;
    UINVARIANT(
        state_.compare_exchange_strong(expected, State::kStopping),
        "Only one shutdown owner may call TimerManager::Shutdown at a time"
    );
    if (operations_.fetch_or(kStoppingBit, std::memory_order_acq_rel) != 0) {
        operations_drained_.WaitNonCancellable();
    }
    Executor::RunInCallbackScope([this] {
        const grpc_core::ApplicationCallbackExecCtx callback_exec_ctx;
        const grpc_core::ExecCtx exec_ctx;
        timer_pool_.storage.Walk([this](TimerPool::TimerData& timerd) { CancelTimer(timerd.GetHandle()); });
    });
    FlushDeferredPayload(ev_thread_pool_);
    state_.store(State::kStopped);
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END

#endif
