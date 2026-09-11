#include <ugrpc/impl/event_engine/timer_manager.hpp>

#include <chrono>
#include <vector>

#include <userver/engine/async.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/rand.hpp>

#include <engine/ev/thread.hpp>
#include <engine/ev/thread_pool.hpp>
#include <engine/task/task_processor.hpp>

USERVER_NAMESPACE_BEGIN

// clang-format off

namespace ugrpc::impl {

namespace {

engine::ev::TimerThreadControl& NextTimerThread(engine::TaskProcessor& task_processor) {
    auto& threads = task_processor.EventThreadPool().GetTimerThreads();
    return threads[utils::RandRange(threads.size())];
}

void FlushDeferredPayload(engine::TaskProcessor& task_processor) {
    UASSERT(engine::current_task::IsTaskProcessorThread());
    auto& threads = task_processor.EventThreadPool().GetTimerThreads();
    std::vector<engine::SingleUseEvent> events{threads.size()};
    for (std::size_t i = 0; i < threads.size(); ++i) {
        auto& event = events[i];
        threads[i].RunInEvLoopAsync([&event] { event.Send(); });
    }
    for (auto& event : events) {
        event.WaitNonCancellable();
    }
}

void TimerCallback(struct ev_loop* /*evloop*/, ev_timer* timer, int) {
    auto& timerd = *static_cast<TimerPool::TimerData*>(timer->data);

    const std::intptr_t revision = timerd.expected;

    // LOG_DEBUG("timerd {} timer rev:{} calling...", static_cast<void*>(&timerd), revision);

    // ev_timer_stop(evloop, timer);
    timerd.stopped = true;

    std::intptr_t expected = revision;
    if (!timerd.revision.compare_exchange_strong(expected, revision + 1)) {
        // LOG_DEBUG("timerd {} timer rev:{} has been cancelled", static_cast<void*>(&timerd), revision);
        return;
    }

    auto closure = std::move(timerd.closure);

    Release(timerd);
    // LOG_DEBUG("timer rev:{} closure running...", revision);

    closure();
}

}  // namespace

void TimerPool::TimerInitPayload::DoPerformAndRelease() {
    if (!timerd_.started.exchange(true)) {
        // LOG_DEBUG("timerd {} timer rev:{} starting", static_cast<void*>(&timerd_), timerd_.expected);
        timerd_.thread_control->Start(timerd_.timer);

        // release
        timerd_.timer_init_payload.reset();
    } else {
        // release
        timerd_.timer_init_payload.reset();
        Release(timerd_);
    }
}

void TimerPool::TimerCancelPayload::DoPerformAndRelease() {
    if (!timerd_.stopped) {
        timerd_.thread_control->Stop(timerd_.timer);
        // LOG_DEBUG("timerd {} timer rev:{} stopped", static_cast<void*>(&timerd_), timerd_.expected);
    }

    // release
    timerd_.timer_cancel_payload.reset();
    Release(timerd_);
}

TimerManager::TimerManager(engine::TaskProcessor& task_processor)
    : task_processor_{task_processor}
{
    // LOG_DEBUG("TimerManager {} created", static_cast<void*>(this));
}

TimerManager::~TimerManager() {
    // LOG_DEBUG("TimerManager {} destroy started", static_cast<void*>(this));

    FlushDeferredPayload(task_processor_);

    // LOG_DEBUG("TimerManager {} destroy complete", static_cast<void*>(this));
}

grpc_event_engine::experimental::EventEngine::TaskHandle TimerManager::TimerInit(
    grpc_event_engine::experimental::EventEngine::Duration when,
    y_absl::AnyInvocable<void()> closure
) {
    // LOG_DEBUG("TimerManager {} TimerInit", static_cast<void*>(this));
    auto& timerd = Acquire(timer_pool_);

    timerd.expected = timerd.revision;

    timerd.closure = std::move(closure);

    timerd.thread_control = &NextTimerThread(task_processor_);

    timerd.timer.data = &timerd;

    const auto after = std::chrono::duration_cast<std::chrono::duration<double>>(when);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_timer_init(&timerd.timer, impl::TimerCallback, /*after*/ after.count(), /*repeat*/ 0.0);

    // LOG_DEBUG("timerd {} timer rev:{} init (after={}ms)", static_cast<void*>(&timerd), revision, duration_cast<std::chrono::milliseconds>(after).count());

    timerd.started = false;
    timerd.stopped = false;

    timerd.timer_init_payload.emplace(timerd);
    timerd.thread_control->RunPayloadInEvLoopDeferred(*timerd.timer_init_payload, engine::Deadline::FromDuration(when));

    return {reinterpret_cast<std::intptr_t>(&timerd), timerd.expected};
}

bool TimerManager::TimerCancel(grpc_event_engine::experimental::EventEngine::TaskHandle handle) {
    // LOG_DEBUG("TimerManager {} TimerCancel", static_cast<void*>(this));
    auto& timerd = *reinterpret_cast<TimerPool::TimerData*>(handle.keys[0]);

    const std::intptr_t revision = handle.keys[1];

    std::intptr_t expected = revision;
    if (!timerd.revision.compare_exchange_strong(expected, revision + 1)) {
        // LOG_DEBUG("engine {} timerd {} timer rev:{} could not be cancelled", static_cast<void*>(this), static_cast<void*>(&timerd), revision);
        return false;
    }

    timerd.closure = nullptr;

    // LOG_DEBUG("timerd {} timer rev:{} cancelled", static_cast<void*>(&timerd), revision);

    if (timerd.started.exchange(true)) {
        timerd.timer_cancel_payload.emplace(timerd);
        timerd.thread_control->RunPayloadInEvLoopDeferred(*timerd.timer_cancel_payload);
    }

    return true;
}

}  // namespace ugrpc::impl

// clang-format on

USERVER_NAMESPACE_END
