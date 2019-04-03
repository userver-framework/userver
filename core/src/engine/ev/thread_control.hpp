#pragma once

#include "thread.hpp"

namespace engine {
namespace ev {

class ThreadControl {
 public:
  explicit ThreadControl(Thread& thread) : thread_(thread) {}
  virtual ~ThreadControl() = default;

  inline struct ev_loop* GetEvLoop() const { return thread_.GetEvLoop(); }

  void AsyncStartUnsafe(ev_async& w) { thread_.AsyncStartUnsafe(w); }

  void AsyncStart(ev_async& w) { thread_.AsyncStart(w); }

  void AsyncStopUnsafe(ev_async& w) { thread_.AsyncStopUnsafe(w); }

  void AsyncStop(ev_async& w) { thread_.AsyncStop(w); }

  void TimerStartUnsafe(ev_timer& w) { thread_.TimerStartUnsafe(w); }

  void TimerStart(ev_timer& w) { thread_.TimerStart(w); }

  void TimerAgainUnsafe(ev_timer& w) { thread_.TimerAgainUnsafe(w); }

  void TimerAgain(ev_timer& w) { thread_.TimerAgain(w); }

  void TimerStopUnsafe(ev_timer& w) { thread_.TimerStopUnsafe(w); }

  void TimerStop(ev_timer& w) { thread_.TimerStop(w); }

  void IoStartUnsafe(ev_io& w) { thread_.IoStartUnsafe(w); }

  void IoStart(ev_io& w) { thread_.IoStart(w); }

  void IoStopUnsafe(ev_io& w) { thread_.IoStopUnsafe(w); }

  void IoStop(ev_io& w) { thread_.IoStop(w); }

  void IdleStartUnsafe(ev_idle& w) { thread_.IdleStartUnsafe(w); }

  void IdleStart(ev_idle& w) { thread_.IdleStart(w); }

  void IdleStopUnsafe(ev_idle& w) { thread_.IdleStopUnsafe(w); }

  void IdleStop(ev_idle& w) { thread_.IdleStop(w); }

  void RunInEvLoopSync(const std::function<void()>& func) {
    thread_.RunInEvLoopSync(func);
  }

  void RunInEvLoopAsync(std::function<void()>&& func) {
    thread_.RunInEvLoopAsync(std::move(func));
  }

  bool IsInEvThread() const { return thread_.IsInEvThread(); }

  Thread& EvThread() const { return thread_; }

 private:
  Thread& thread_;
};

}  // namespace ev
}  // namespace engine
