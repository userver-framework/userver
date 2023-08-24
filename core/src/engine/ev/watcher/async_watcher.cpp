#include "async_watcher.hpp"

#include <userver/engine/async.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

AsyncWatcher::AsyncWatcher(ThreadControl& thread_control, Callback cb)
    : ev_async_(thread_control, this), cb_(std::move(cb)) {
  ev_async_.Init(&AsyncWatcher::OnEvent);
}

AsyncWatcher::~AsyncWatcher() = default;

void AsyncWatcher::Start() { ev_async_.Start(); }

void AsyncWatcher::OnEvent(struct ev_loop*, ev_async* async,
                           int events) noexcept {
  auto* self = static_cast<AsyncWatcher*>(async->data);
  self->ev_async_.Stop();

  if (events & EV_ASYNC) {
    try {
      self->CallCb();
    } catch (const std::exception& ex) {
      LOG_ERROR() << "Uncaught exception in AsyncWatcher callback: " << ex;
    }
  }
}

void AsyncWatcher::Send() { ev_async_.Send(); }

void AsyncWatcher::CallCb() {
  LOG_TRACE() << "CallCb (1) watcher=" << this;

  cb_();
}

}  // namespace engine::ev

USERVER_NAMESPACE_END
