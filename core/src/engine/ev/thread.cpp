#include "thread.hpp"

#include <chrono>
#include <stdexcept>

#include <userver/compiler/demangle.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/datetime/steady_coarse_clock.hpp>
#include <userver/utils/thread_name.hpp>

#include <utils/check_syscall.hpp>
#include <utils/statistics/thread_statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {
namespace {

constexpr std::chrono::milliseconds kCpuStatsCollectInterval{1000};

const auto kDeferredInterval = kMinDurationToDefer - utils::datetime::SteadyCoarseClock::resolution();

// Check the time at least twice per collect interval
const auto kCpuStatsThrottle = static_cast<std::size_t>(kCpuStatsCollectInterval / kDeferredInterval / 2);

}  // namespace

Thread::Thread(const std::string& thread_name) : Thread(thread_name, EventLoop::EvLoopType::kNewLoop) {}

Thread::Thread(const std::string& thread_name, UseDefaultEvLoop)
    : Thread(thread_name, EventLoop::EvLoopType::kDefaultLoop) {}

Thread::Thread(const std::string& thread_name, EventLoop::EvLoopType ev_loop_type)
    : event_loop_(ev_loop_type), name_{thread_name}, cpu_stats_storage_{kCpuStatsCollectInterval, kCpuStatsThrottle} {
    UASSERT_MSG(kDeferredInterval > std::chrono::milliseconds{4}, "Timer events would happen too often");
    Start();
}

Thread::~Thread() { StopEventLoop(); }

void Thread::RunInEvLoopAsync(AsyncPayloadBase& payload) noexcept {
    RegisterInEvLoop(payload);

    if (!IsInEvThread()) {
        ev_async_send(GetEvLoop(), &watch_update_);
    }
}

void Thread::RunInEvLoopDeferred(AsyncPayloadBase& payload, Deadline deadline) noexcept {
    UASSERT_MSG(
        deadline.IsReachable(),
        "RunInEvLoopDeferred with unreachable deadline may produce unpredictable "
        "latency. Use RunInEvLoopAsync or provide a reachable dedaline"
    );
    RegisterInEvLoop(payload);

    if (IsInEvThread() || ev_async_pending(&watch_update_)) {
        return;
    }

    const auto time_left = deadline.TimeLeftApprox();
    if (time_left >= kMinDurationToDefer) {
        return;
    }

    ev_async_send(GetEvLoop(), &watch_update_);
}

void Thread::RegisterInEvLoop(AsyncPayloadBase& payload) {
    if (IsInEvThread()) {
        payload.PerformAndRelease();
        return;
    }

    func_queue_.Push(payload);
}

bool Thread::IsInEvThread() const { return (std::this_thread::get_id() == thread_.get_id()); }

std::uint8_t Thread::GetCurrentLoadPercent() const { return cpu_stats_storage_.GetCurrentLoadPercent(); }

const std::string& Thread::GetName() const { return name_; }

void Thread::Start() {
    auto* loop = GetEvLoop();

    UASSERT(loop);
    ev_set_userdata(loop, this);
    ev_set_loop_release_cb(loop, Release, Acquire);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_async_init(&watch_update_, UpdateLoopWatcher);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_set_priority(&watch_update_, 1);
    ev_async_start(loop, &watch_update_);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_async_init(&watch_break_, BreakLoopWatcher);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_set_priority(&watch_break_, EV_MAXPRI);
    ev_async_start(loop, &watch_break_);

    using LibEvDuration = std::chrono::duration<double>;
    const auto defer_duration = std::chrono::duration_cast<LibEvDuration>(kDeferredInterval);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_timer_init(&defer_timer_, UpdateTimersWatcher, 0.0, defer_duration.count());
    ev_timer_start(loop, &defer_timer_);

    is_running_ = true;
    thread_ = std::thread([this] {
        utils::SetCurrentThreadName(name_);
        RunEvLoop();
    });
}

void Thread::StopEventLoop() {
    ev_async_send(GetEvLoop(), &watch_break_);
    if (thread_.joinable()) thread_.join();

    if (func_queue_.TryPopBlocking()) {
        utils::impl::AbortWithStacktrace("Some work was enqueued on a dead Thread");
    }
}

void Thread::RunEvLoop() {
    while (is_running_) {
        AcquireImpl();
        event_loop_.RunOnce();
        UpdateLoopWatcherImpl();
        cpu_stats_storage_.Collect();
        ReleaseImpl();
    }

    ev_async_stop(GetEvLoop(), &watch_update_);
    ev_async_stop(GetEvLoop(), &watch_break_);
    ev_timer_stop(GetEvLoop(), &defer_timer_);
}

void Thread::UpdateLoopWatcher(struct ev_loop* loop, ev_async*, int) noexcept {
    auto* ev_thread = static_cast<Thread*>(ev_userdata(loop));
    UASSERT(ev_thread != nullptr);
    ev_thread->UpdateLoopWatcherImpl();
}

void Thread::UpdateTimersWatcher(struct ev_loop* loop, ev_timer*, int) noexcept {
    auto* ev_thread = static_cast<Thread*>(ev_userdata(loop));
    UASSERT(ev_thread != nullptr);
    ev_thread->UpdateLoopWatcherImpl();
}

void Thread::UpdateLoopWatcherImpl() {
    while (AsyncPayloadBase* payload = func_queue_.TryPopBlocking()) {
        LOG_TRACE() << "Thread::UpdateLoopWatcherImpl(), " << compiler::GetTypeName(typeid(*payload));
        try {
            payload->PerformAndRelease();
        } catch (const std::exception& ex) {
            LOG_WARNING() << "exception in async thread func: " << ex;
        }
    }
}

void Thread::BreakLoopWatcher(struct ev_loop* loop, ev_async*, int) noexcept {
    auto* ev_thread = static_cast<Thread*>(ev_userdata(loop));
    UASSERT(ev_thread != nullptr);
    ev_thread->BreakLoopWatcherImpl();
}

void Thread::BreakLoopWatcherImpl() {
    is_running_ = false;
    UpdateLoopWatcherImpl();
    ev_break(GetEvLoop(), EVBREAK_ALL);
}

void Thread::Acquire(struct ev_loop* loop) noexcept {
    auto* ev_thread = static_cast<Thread*>(ev_userdata(loop));
    UASSERT(ev_thread != nullptr);
    ev_thread->AcquireImpl();
}

void Thread::Release(struct ev_loop* loop) noexcept {
    auto* ev_thread = static_cast<Thread*>(ev_userdata(loop));
    UASSERT(ev_thread != nullptr);
    ev_thread->ReleaseImpl();
}

void Thread::AcquireImpl() noexcept { lock_.lock(); }
void Thread::ReleaseImpl() noexcept { lock_.unlock(); }

}  // namespace engine::ev

USERVER_NAMESPACE_END
