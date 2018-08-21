#include "timer_watcher.hpp"

#include <engine/async.hpp>

namespace engine {
namespace ev {

TimerWatcher::TimerWatcher(engine::ev::ThreadControl& thread_control)
    : ev_timer_(thread_control, this) {}

TimerWatcher::~TimerWatcher() { Cancel(); }

void TimerWatcher::SingleshotAsync(std::chrono::milliseconds timeout,
                                   Callback cb) {
  LOG_TRACE() << "TimerWatcher::SingleshotAsync";
  Cancel();
  {
    std::lock_guard<std::mutex> lock(mutex_);
    swap(cb, cb_);
  }

  ev_timer_.Init(&TimerWatcher::OnEventTimeout, (1.0 * timeout.count()) / 1000,
                 0.);
  ev_timer_.Start();
}

void TimerWatcher::OnEventTimeout(struct ev_loop*, ev_timer* timer,
                                  int events) {
  auto self = static_cast<TimerWatcher*>(timer->data);
  self->ev_timer_.Stop();

  if (events & EV_TIMER) {
    self->CallTimeoutCb(std::error_code());
  }
}

void TimerWatcher::CallTimeoutCb(std::error_code ec) {
  LOG_TRACE() << "TimerWatcher::CallTimeoutCb  watcher="
              << reinterpret_cast<long>(this);
  Callback cb;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    swap(cb, cb_);
  }

  if (cb) {
    cb(ec);
  }
}

void TimerWatcher::Cancel() {
  LOG_TRACE() << "TimerWatcher::Cancel() (1) watcher="
              << reinterpret_cast<long>(this);
  if (cb_) {
    LOG_TRACE() << "TimerWatcher::Cancel() (2) watcher="
                << reinterpret_cast<long>(this);
    ev_timer_.Stop();
    LOG_TRACE() << "TimerWatcher::Cancel() (2.1) watcher="
                << reinterpret_cast<long>(this);
    CallTimeoutCb(std::make_error_code(std::errc::operation_canceled));
  }
  LOG_TRACE() << "TimerWatcher::Cancel() (3) watcher="
              << reinterpret_cast<long>(this);
}

}  // namespace ev
}  // namespace engine
