#pragma once

#include <functional>

#include <engine/ev/thread_control.hpp>
#include <engine/ev/watcher.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

class AsyncWatcher final {
 public:
  using Callback = std::function<void()>;
  explicit AsyncWatcher(ThreadControl& thread_control, Callback cb);
  AsyncWatcher(const AsyncWatcher&) = delete;
  ~AsyncWatcher();

  void Start();

  void Send();

 private:
  static void OnEvent(struct ev_loop* loop, ev_async* async,
                      int events) noexcept;
  void CallCb();

  Watcher<ev_async> ev_async_;
  const Callback cb_;
};

}  // namespace engine::ev

USERVER_NAMESPACE_END
