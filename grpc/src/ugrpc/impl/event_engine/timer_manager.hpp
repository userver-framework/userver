#pragma once

#include <grpc/event_engine/event_engine.h>

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/logging/log.hpp>

#include <concurrent/intrusive_walkable_pool.hpp>

#include <engine/ev/thread_control.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

struct TimerPool {
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
        std::atomic<std::intptr_t> revision{0};
        std::intptr_t expected{0};

        y_absl::AnyInvocable<void()> closure;

        engine::ev::TimerThreadControl* thread_control{};

        ev_timer timer{};

        std::optional<TimerInitPayload> timer_init_payload;
        std::optional<TimerCancelPayload> timer_cancel_payload;

        std::atomic<bool> started;
        bool stopped{};

        TimerPool* timer_pool{};

        concurrent::impl::IntrusiveWalkablePoolHook<TimerData> hook;
    };

    using Pool = concurrent::impl::IntrusiveWalkablePool<
        TimerData,
        concurrent::impl::MemberHook<&TimerData::hook>,
        offsetof(TimerData, hook)>;

    Pool pool;
};

inline TimerPool::TimerData& Acquire(TimerPool& timer_pool) {
    auto& timerd = timer_pool.pool.Acquire();

    timerd.timer_pool = &timer_pool;

    return timerd;
}

inline void Release(TimerPool::TimerData& timerd) {
    TimerPool& timer_pool = *timerd.timer_pool;
    timerd.timer_pool = nullptr;

    UASSERT(!timerd.timer_cancel_payload.has_value());
    UASSERT(!timerd.timer_init_payload.has_value());

    UASSERT(!timerd.closure);

    timer_pool.pool.Release(timerd);
}

class TimerManager final {
public:
    explicit TimerManager(engine::TaskProcessor& task_processor);

    ~TimerManager();

    grpc_event_engine::experimental::EventEngine::TaskHandle TimerInit(
        grpc_event_engine::experimental::EventEngine::Duration when,
        y_absl::AnyInvocable<void()> closure
    );

    bool TimerCancel(grpc_event_engine::experimental::EventEngine::TaskHandle handle);

private:
    engine::TaskProcessor& task_processor_;

    TimerPool timer_pool_;
};

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
