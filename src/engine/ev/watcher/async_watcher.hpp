#pragma once

#include <engine/ev/thread_control.hpp>
#include <engine/task/task_processor.hpp>
#include <engine/watcher.hpp>

namespace engine {
namespace ev {

class AsyncWatcher {
 public:
  using Callback = std::function<void()>;
  explicit AsyncWatcher(engine::ev::ThreadControl& thread_control, Callback cb);
  AsyncWatcher(const AsyncWatcher&) = delete;
  ~AsyncWatcher();

  void Start();

  void Send();

 private:
  static void OnEvent(struct ev_loop* loop, ev_async* timer, int events);
  void CallCb();

 private:
  engine::Watcher<ev_async> ev_async_;
  const Callback cb_;
  std::mutex mutex_;
};

}  // namespace ev
}  // namespace engine
