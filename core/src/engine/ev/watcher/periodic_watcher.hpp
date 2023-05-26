#pragma once

#include <functional>
#include <system_error>

#include <engine/ev/thread_control.hpp>
#include <engine/ev/watcher.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

class PeriodicWatcher {
 public:
  PeriodicWatcher(engine::ev::ThreadControl ev_thread,
                  std::function<void()> action, std::chrono::seconds interval);

  void Start();
  void Stop();

 private:
  static void OnTimer(struct ev_loop*, ev_timer* w, int /*revents*/) noexcept;

  engine::ev::Watcher<ev_timer> timer_;
  std::function<void()> action_;
  std::chrono::seconds interval_;
};

}  // namespace engine::ev

USERVER_NAMESPACE_END
