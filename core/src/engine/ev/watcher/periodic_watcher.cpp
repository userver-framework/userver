#include "periodic_watcher.hpp"

#include <userver/engine/async.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

PeriodicWatcher::PeriodicWatcher(engine::ev::ThreadControl ev_thread,
                                 std::function<void()> action,
                                 std::chrono::seconds interval)
    : timer_(ev_thread, this), action_(std::move(action)), interval_(interval) {
  timer_.Init(&PeriodicWatcher::OnTimer, {}, {});
  timer_.Set({}, interval_);
}

void PeriodicWatcher::Start() { timer_.Start(); }

void PeriodicWatcher::Stop() { timer_.Stop(); }

void PeriodicWatcher::OnTimer(struct ev_loop*, ev_timer* w,
                              int /*revents*/) noexcept {
  auto* impl = static_cast<PeriodicWatcher*>(w->data);
  UASSERT(impl != nullptr);
  try {
    impl->action_();
  } catch (const std::exception& ex) {
    LOG_ERROR() << "Uncaught exception in PeriodicWatcher callback: " << ex;
  }
}

}  // namespace engine::ev

USERVER_NAMESPACE_END
