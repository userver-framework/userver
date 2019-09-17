#include "timer_watcher.hpp"

#include <cstdlib>
#include <iostream>

#include <engine/async.hpp>
#include <utils/userver_experiment.hpp>

namespace engine {
namespace ev {

TimerWatcher::TimerWatcher(ThreadControl& thread_control)
    : ev_timer_(thread_control, this) {}

// NOLINTNEXTLINE(bugprone-exception-escape)
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

// NOLINTNEXTLINE(bugprone-exception-escape)
void TimerWatcher::OnEventTimeout(struct ev_loop*, ev_timer* timer,
                                  int events) try {
  auto self = static_cast<TimerWatcher*>(timer->data);
  self->ev_timer_.Stop();

  if (events & EV_TIMER) {
    try {
      self->CallTimeoutCb(std::error_code());
    } catch (const std::exception& ex) {
      LOG_ERROR() << "Uncaught exception in TimerWatcher callback: " << ex;
    }
  }
} catch (...) {
  if (utils::IsUserverExperimentEnabled(
          utils::UserverExperiment::kTaxicommon1479)) {
    std::cerr << "Uncaught exception in " << __PRETTY_FUNCTION__;
    abort();
  }
  throw;
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
  bool need_call_cb = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (cb_) need_call_cb = true;
  }
  if (need_call_cb) {
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
