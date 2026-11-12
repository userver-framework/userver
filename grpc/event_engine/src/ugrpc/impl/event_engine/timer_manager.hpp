#pragma once

#if defined(ARCADIA_ROOT) && defined(__linux__)

#include <grpc/event_engine/event_engine.h>
#include <y_absl/functional/any_invocable.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <optional>

#include <ev.h>

#include <userver/concurrent/impl/intrusive_hooks.hpp>
#include <userver/engine/single_use_event.hpp>
#include <userver/utils/assert.hpp>

#include <engine/ev/async_payload_base.hpp>
#include <engine/ev/thread_control.hpp>

#include <ugrpc/impl/event_engine/timer_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {
class ThreadPool;
}

namespace ugrpc::impl {

class Executor;

struct TimerPool {
    using TaskHandle = grpc_event_engine::experimental::EventEngine::TaskHandle;

    explicit TimerPool(Executor& callback_executor)
        : executor(callback_executor)
    {}

    struct TimerData;

    class TimerInitPayload final : public engine::ev::SingleShotAsyncPayload<TimerInitPayload> {
    public:
        explicit TimerInitPayload(TimerData& timerd)
            : timerd_{timerd}
        {}

        void DoPerformAndRelease();

    private:
        TimerData& timerd_;
    };

    class TimerCancelPayload final : public engine::ev::SingleShotAsyncPayload<TimerCancelPayload> {
    public:
        explicit TimerCancelPayload(TimerData& timerd)
            : timerd_{timerd}
        {}

        void DoPerformAndRelease();

    private:
        TimerData& timerd_;
    };

    struct TimerData {
        explicit TimerData(TimerPool& timer_pool)
            : timer_pool(&timer_pool)
        {}

        TaskHandle GetHandle() const;

        std::atomic<std::intptr_t> revision{0};
        std::intptr_t expected{0};

        y_absl::AnyInvocable<void()> closure;

        engine::ev::TimerThreadControl* thread_control{};
        std::chrono::steady_clock::time_point deadline{};

        ev_timer timer{};

        std::optional<TimerInitPayload> timer_init_payload;
        std::optional<TimerCancelPayload> timer_cancel_payload;

        std::atomic<bool> started{false};
        bool stopped{};

        TimerPool* const timer_pool;

        event_engine::TimerStorageHook<TimerData> hook;
    };

    Executor& executor;
    event_engine::TimerStorage<TimerData, concurrent::impl::MemberHook<&TimerData::hook>> storage;
};

inline TimerPool::TimerData& Acquire(TimerPool& timer_pool) {
    return timer_pool.storage.Acquire([&] { return TimerPool::TimerData{timer_pool}; });
}

inline void Release(TimerPool::TimerData& timerd) {
    UASSERT(!timerd.timer_cancel_payload.has_value());
    UASSERT(!timerd.timer_init_payload.has_value());
    UASSERT(!timerd.closure);
    UASSERT(!ev_is_active(&timerd.timer));
    timerd.timer_pool->storage.Release(timerd);
}

/// Native libev timers with callbacks dispatched to Executor.
/// The EV pool and executor must outlive Shutdown(). Borrowed callback resources must
/// remain alive until execution finishes, normally through executor quiescence.
class TimerManager final {
public:
    using Duration = grpc_event_engine::experimental::EventEngine::Duration;
    using TaskHandle = grpc_event_engine::experimental::EventEngine::TaskHandle;
    using AnyInvocable = y_absl::AnyInvocable<void()>;

    TimerManager(engine::ev::ThreadPool& ev_thread_pool, Executor& executor);

    TimerManager(const TimerManager&) = delete;
    TimerManager& operator=(const TimerManager&) = delete;

    /// Requires a coroutine unless Shutdown() has already completed.
    ~TimerManager();

    /// Concurrent. Transfers ownership until execution or successful cancellation.
    /// Nonpositive delays dispatch directly and return TaskHandle::kInvalid, as upstream does.
    /// Positive delays registered after Shutdown begins destroy captures in callback scope without running.
    TaskHandle TimerInit(Duration when, AnyInvocable closure);

    /// Concurrent. Success destroys owned captures before returning on the caller's thread
    /// inside callback scope. The handle must be kInvalid or belong to this still-alive manager.
    /// Returns false once shutdown has taken ownership of the remaining timers.
    bool TimerCancel(TaskHandle handle);

    /// Closes delayed-timer admission and waits for admitted initialization and cancellation calls.
    /// Cancels pending timers and flushes EV payloads; does not wait for committed callbacks.
    /// Call from one coroutine outside callbacks/capture destruction, then quiesce the executor.
    /// Idempotent after completion.
    void Shutdown();

private:
    enum class State { kRunning, kStopping, kStopped };

    static void OnTimer(struct ev_loop*, ev_timer* timer, int);

    bool TryStartOperation() noexcept;
    void FinishOperation() noexcept;
    bool CancelTimer(TaskHandle handle);

    static constexpr std::uint64_t kStoppingBit = std::uint64_t{1} << 63;

    engine::ev::ThreadPool& ev_thread_pool_;
    TimerPool timer_pool_;
    std::atomic<State> state_{State::kRunning};
    std::atomic<std::uint64_t> operations_{0};
    engine::SingleUseEvent operations_drained_;
};

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END

#endif
