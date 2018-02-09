#pragma once

#include <chrono>

#include "thread_control.hpp"

#include <logger/logger.hpp>

namespace engine {
namespace ev {

template <typename CurrentTask>
class Timer {
 public:
  using Func = std::function<void()>;

  template <typename Rep1, typename Period1, typename Rep2, typename Period2>
  Timer(ThreadControl& thread_control, Func&& on_timer_func,
        const std::chrono::duration<Rep1, Period1>& first_call_after,
        const std::chrono::duration<Rep2, Period2>& repeat_every,
        bool start_now = true);

  template <typename Rep, typename Period>
  Timer(ThreadControl& thread_control, Func&& on_timer_func,
        const std::chrono::duration<Rep, Period>& call_after,
        bool start_now = true);

  template <typename Rep1, typename Period1, typename Rep2, typename Period2>
  Timer(Func&& on_timer_func,
        const std::chrono::duration<Rep1, Period1>& first_call_after,
        const std::chrono::duration<Rep2, Period2>& repeat_every,
        bool start_now = true);

  template <typename Rep, typename Period>
  Timer(Func&& on_timer_func,
        const std::chrono::duration<Rep, Period>& call_after,
        bool start_now = true);

  Timer(const Timer&) = delete;
  Timer& operator=(const Timer&) = delete;

  ~Timer() { Stop(); }

  ev_timer& GetTimer() { return timer_; }

  void Start();
  bool Started() const { return started_; }

 private:
  static void OnTimer(struct ev_loop*, ev_timer* w, int);
  void OnTimerImpl();
  void Init(bool start_now);
  void Stop();

  Func on_timer_func_;
  double first_call_after_;
  double repeat_every_;
  ThreadControl thread_control_;
  ev_timer timer_;
  bool started_;
};

template <typename CurrentTask>
template <typename Rep1, typename Period1, typename Rep2, typename Period2>
Timer<CurrentTask>::Timer(
    ThreadControl& thread_control, Func&& on_timer_func,
    const std::chrono::duration<Rep1, Period1>& first_call_after,
    const std::chrono::duration<Rep2, Period2>& repeat_every, bool start_now)
    : on_timer_func_(std::move(on_timer_func)),
      first_call_after_(
          std::chrono::duration_cast<std::chrono::duration<double>>(
              first_call_after)
              .count()),
      repeat_every_(std::chrono::duration_cast<std::chrono::duration<double>>(
                        repeat_every)
                        .count()),
      thread_control_(thread_control),
      started_(false) {
  Init(start_now);
}

template <typename CurrentTask>
template <typename Rep, typename Period>
Timer<CurrentTask>::Timer(ThreadControl& thread_control, Func&& on_timer_func,
                          const std::chrono::duration<Rep, Period>& call_after,
                          bool start_now)
    : Timer(thread_control, std::move(on_timer_func), call_after,
            std::chrono::seconds(0), start_now) {}

template <typename CurrentTask>
template <typename Rep1, typename Period1, typename Rep2, typename Period2>
Timer<CurrentTask>::Timer(
    Func&& on_timer_func,
    const std::chrono::duration<Rep1, Period1>& first_call_after,
    const std::chrono::duration<Rep2, Period2>& repeat_every, bool start_now)
    : Timer(CurrentTask::GetSchedulerThread(), std::move(on_timer_func),
            first_call_after, repeat_every, start_now) {}

template <typename CurrentTask>
template <typename Rep, typename Period>
Timer<CurrentTask>::Timer(Func&& on_timer_func,
                          const std::chrono::duration<Rep, Period>& call_after,
                          bool start_now)
    : Timer(CurrentTask::GetSchedulerThread(), std::move(on_timer_func),
            call_after, std::chrono::seconds(0), start_now) {}

template <typename CurrentTask>
void Timer<CurrentTask>::Start() {
  assert(!started_);
  started_ = true;
  thread_control_.TimerStart(timer_);
}

template <typename CurrentTask>
void Timer<CurrentTask>::OnTimer(struct ev_loop*, ev_timer* w, int) {
  Timer* ev_timer = static_cast<Timer*>(w->data);
  assert(ev_timer != nullptr);
  ev_timer->OnTimerImpl();
}

template <typename CurrentTask>
void Timer<CurrentTask>::OnTimerImpl() {
  try {
    on_timer_func_();
  } catch (const std::exception& ex) {
    LOG_WARNING() << "exception in on_timer_func: " << ex.what();
  }
}

template <typename CurrentTask>
void Timer<CurrentTask>::Init(bool start_now) {
  if (first_call_after_ < 0.0) first_call_after_ = 0.0;
  timer_.data = this;
  ev_timer_init(&timer_, OnTimer, first_call_after_, repeat_every_);
  LOG_TRACE() << "first_call_after_=" << first_call_after_
              << " repeat_every_=" << repeat_every_;
  if (start_now) Start();
}

template <typename CurrentTask>
void Timer<CurrentTask>::Stop() {
  if (started_) {
    thread_control_.TimerStop(timer_);
    started_ = false;
  }
}

}  // namespace ev
}  // namespace engine
