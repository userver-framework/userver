#pragma once

#include <engine/ev/thread_control.hpp>
#include <engine/ev/watcher.hpp>
#include <engine/task/task_processor.hpp>

namespace engine {
namespace ev {

class AsyncWatcher final {
 public:
  using Callback = std::function<void()>;
  explicit AsyncWatcher(ThreadControl& thread_control, Callback cb);
  AsyncWatcher(const AsyncWatcher&) = delete;
  ~AsyncWatcher();

  void Start();

  void Send();

 private:
  static void OnEvent(struct ev_loop* loop, ev_async* async, int events);
  void CallCb();

 private:
  Watcher<ev_async> ev_async_;
  const Callback cb_;
  std::mutex mutex_;
};

}  // namespace ev
}  // namespace engine
