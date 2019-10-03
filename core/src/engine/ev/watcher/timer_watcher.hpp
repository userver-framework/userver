#pragma once

#include <engine/ev/thread_control.hpp>
#include <engine/ev/watcher.hpp>
#include <engine/task/task_processor.hpp>

namespace engine {
namespace ev {

class TimerWatcher final {
 public:
  explicit TimerWatcher(ThreadControl& thread_control);

  TimerWatcher(const TimerWatcher&) = delete;
  // NOLINTNEXTLINE(bugprone-exception-escape)
  ~TimerWatcher();

  void Cancel();

  using Callback = std::function<void(std::error_code)>;
  void SingleshotAsync(std::chrono::milliseconds timeout, Callback cb);

 private:
  static void OnEventTimeout(struct ev_loop* loop, ev_timer* timer,
                             int events) noexcept;
  void CallTimeoutCb(std::error_code ec);

 private:
  Watcher<ev_timer> ev_timer_;
  Callback cb_;
  std::mutex mutex_;
};

}  // namespace ev
}  // namespace engine
