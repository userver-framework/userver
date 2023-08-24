#include "timer_watcher.hpp"

#include <userver/engine/async.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

TimerWatcher::TimerWatcher(ThreadControl& thread_control)
    : ev_timer_(thread_control, this) {
  ev_timer_.Init(&TimerWatcher::OnEventTimeout, {}, {});
}

TimerWatcher::~TimerWatcher() { Cancel(); }

void TimerWatcher::SingleshotAsync(std::chrono::milliseconds timeout,
                                   Callback cb) {
  LOG_TRACE() << "TimerWatcher::SingleshotAsync";
  Cancel();
  {
    const std::lock_guard lock{mutex_};
    swap(cb, cb_);
  }

  ev_timer_.Set(timeout, {});
  ev_timer_.Start();
}

void TimerWatcher::OnEventTimeout(struct ev_loop*, ev_timer* timer,
                                  int events) noexcept {
  auto* self = static_cast<TimerWatcher*>(timer->data);
  self->ev_timer_.Stop();

  if (events & EV_TIMER) {
    try {
      self->CallTimeoutCb(std::error_code());
    } catch (const std::exception& ex) {
      LOG_ERROR() << "Uncaught exception in TimerWatcher callback: " << ex;
    }
  }
}

void TimerWatcher::CallTimeoutCb(std::error_code ec) {
  LOG_TRACE() << "TimerWatcher::CallTimeoutCb  watcher=" << this;
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
  LOG_TRACE() << "TimerWatcher::Cancel() (1) watcher=" << this;
  bool need_call_cb = false;
  {
    const std::lock_guard lock{mutex_};
    if (cb_) need_call_cb = true;
  }
  if (need_call_cb) {
    LOG_TRACE() << "TimerWatcher::Cancel() (2) watcher=" << this;
    ev_timer_.Stop();
    LOG_TRACE() << "TimerWatcher::Cancel() (2.1) watcher=" << this;
    CallTimeoutCb(std::make_error_code(std::errc::operation_canceled));
  }
  LOG_TRACE() << "TimerWatcher::Cancel() (3) watcher=" << this;
}

}  // namespace engine::ev

USERVER_NAMESPACE_END
