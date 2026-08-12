#include "timer_watcher.hpp"

#include <userver/engine/async.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

TimerWatcher::TimerWatcher(ThreadControl& thread_control)
    : ev_timer_(thread_control, this)
{
    ev_timer_.Init(&TimerWatcher::OnEventTimeout, {}, {});
}

TimerWatcher::~TimerWatcher() { Cancel(); }

void TimerWatcher::SingleshotAsync(std::chrono::milliseconds timeout, Callback cb) {
    Cancel();
    {
        const std::lock_guard lock{mutex_};
        swap(cb, cb_);
    }

    ev_timer_.Set(timeout, {});
    ev_timer_.StartAsync();
}

void TimerWatcher::OnEventTimeout(struct ev_loop*, ev_timer* timer, int events) noexcept {
    auto* self = static_cast<TimerWatcher*>(timer->data);
    const auto guard = self->ev_timer_.StopWithinEvCallback();

    if (events & EV_TIMER) {
        try {
            self->CallTimeoutCb(std::error_code());
        } catch (const std::exception& ex) {
            LOG_ERROR() << "Uncaught exception in TimerWatcher callback: " << ex;
        }
    }
}

void TimerWatcher::CallTimeoutCb(std::error_code ec) {
    Callback cb;
    {
        const std::lock_guard lock{mutex_};
        swap(cb, cb_);
    }

    if (cb) {
        cb(ec);
    }
}

void TimerWatcher::Cancel() {
    bool need_call_cb = false;
    {
        const std::lock_guard lock{mutex_};
        if (cb_) {
            need_call_cb = true;
        }
    }
    if (need_call_cb) {
        ev_timer_.Stop();
        CallTimeoutCb(std::make_error_code(std::errc::operation_canceled));
    }
}

}  // namespace engine::ev

USERVER_NAMESPACE_END
