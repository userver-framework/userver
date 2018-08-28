#pragma once

#include <engine/ev/thread_control.hpp>
#include <engine/task/task_processor.hpp>
#include <engine/watcher.hpp>

namespace engine {
namespace ev {

class TimerWatcher {
 public:
  explicit TimerWatcher(engine::ev::ThreadControl& thread_control);

  TimerWatcher(const TimerWatcher&) = delete;
  ~TimerWatcher();

  void Cancel();

  using Callback = std::function<void(std::error_code)>;
  void SingleshotAsync(std::chrono::milliseconds timeout, Callback cb);

 private:
  static void OnEventTimeout(struct ev_loop* loop, ev_timer* timer, int events);
  void CallTimeoutCb(std::error_code ec);

 private:
  engine::Watcher<ev_timer> ev_timer_;
  Callback cb_;
  std::mutex mutex_;
};

}  // namespace ev
}  // namespace engine
