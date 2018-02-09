#pragma once

#include <cassert>
#include <chrono>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include <ev.h>
#include <boost/lockfree/queue.hpp>

namespace engine {
namespace ev {

class Thread {
 public:
  explicit Thread(const std::string& thread_name);
  ~Thread();

  struct ev_loop* GetEvLoop() const {
    return loop_;
  }

  void SetThreadName(const std::string& name);
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
  void RunInEvLoopSync(const std::function<void()>& func);
  void RunInEvLoopAsync(std::function<void()>&& func);

 private:
  template <typename Func>
  void SafeEvCall(const Func& func);

  void Run(const std::string& thread_name);

  void StopEventLoop();
  void UpdateEvLoop();
  void RunEvLoop();
  bool CheckThread() const;

  static void UpdateLoopWatcher(struct ev_loop*, ev_async* w, int);
  void UpdateLoopWatcherImpl();
  static void BreakLoopWatcher(struct ev_loop*, ev_async* w, int);
  void BreakLoopWatcherImpl();

  static void Release(struct ev_loop* loop);
  static void Acquire(struct ev_loop* loop);
  void ReleaseImpl();
  void AcquireImpl();

  const std::function<void()>* func_ptr_;
  std::unique_ptr<std::promise<void>> func_promise_;
  boost::lockfree::queue<std::function<void()>*> func_queue_;

  struct ev_loop* loop_;
  std::thread thread_;
  std::mutex mutex_;
  std::mutex loop_mutex_;
  std::unique_lock<std::mutex> lock_;
  ev_async watch_update_;
  ev_async watch_break_;
  bool running_;
};

}  // namespace ev
}  // namespace engine
