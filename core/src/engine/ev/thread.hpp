#pragma once

#include <atomic>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <ev.h>
#include <boost/lockfree/queue.hpp>

namespace engine {
namespace ev {

class Thread final {
 public:
  struct UseDefaultEvLoop {};
  static constexpr UseDefaultEvLoop kUseDefaultEvLoop;

  explicit Thread(const std::string& thread_name);
  Thread(const std::string& thread_name, UseDefaultEvLoop);
  ~Thread();

  struct ev_loop* GetEvLoop() const {
    return loop_;
  }

  void AsyncStartUnsafe(ev_async& w);
  void AsyncStart(ev_async& w);
  void AsyncStopUnsafe(ev_async& w);
  void AsyncStop(ev_async& w);
  void TimerStartUnsafe(ev_timer& w);
  void TimerStart(ev_timer& w);
  void TimerAgainUnsafe(ev_timer& w);
  void TimerAgain(ev_timer& w);
  void TimerStopUnsafe(ev_timer& w);
  void TimerStop(ev_timer& w);
  void IoStartUnsafe(ev_io& w);
  void IoStart(ev_io& w);
  void IoStopUnsafe(ev_io& w);
  void IoStop(ev_io& w);
  void IdleStartUnsafe(ev_idle& w);
  void IdleStart(ev_idle& w);
  void IdleStopUnsafe(ev_idle& w);
  void IdleStop(ev_idle& w);

  // Callbacks passed to RunInEvLoopAsync() are serialized.
  void RunInEvLoopAsync(std::function<void()>&& func);

  bool IsInEvThread() const;

 private:
  Thread(const std::string& thread_name, bool use_ev_default_loop);

  template <typename Func>
  void SafeEvCall(const Func& func);

  void Start();

  void StopEventLoop();
  void RunEvLoop();

  static void UpdateLoopWatcher(struct ev_loop*, ev_async* w, int) noexcept;
  void UpdateLoopWatcherImpl();
  static void BreakLoopWatcher(struct ev_loop*, ev_async* w, int) noexcept;
  void BreakLoopWatcherImpl();
  static void ChildWatcher(struct ev_loop*, ev_child* w, int) noexcept;
  void ChildWatcherImpl(ev_child* w);

  static void Acquire(struct ev_loop* loop) noexcept;
  static void Release(struct ev_loop* loop) noexcept;
  void AcquireImpl() noexcept;
  void ReleaseImpl() noexcept;

  bool use_ev_default_loop_;

  boost::lockfree::queue<std::function<void()>*> func_queue_;

  struct ev_loop* loop_;
  std::thread thread_;
  std::mutex loop_mutex_;
  std::unique_lock<std::mutex> lock_;
  ev_async watch_update_{};
  ev_async watch_break_{};
  ev_child watch_child_{};
  bool is_running_;
};

}  // namespace ev
}  // namespace engine
