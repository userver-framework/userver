#pragma once

#include "async.hpp"
#include "timer_event.hpp"

namespace engine {

class Timer {
 public:
  // calls on_timer_func() in a new coroutine
  using Func = impl::TimerEvent::Func;
  using StartMode = impl::TimerEvent::StartMode;

  template <typename Rep1, typename Period1, typename Rep2, typename Period2>
  Timer(ev::ThreadControl& thread_control, Func on_timer_func,
        const std::chrono::duration<Rep1, Period1>& first_call_after,
        const std::chrono::duration<Rep2, Period2>& repeat_every,
        StartMode start_mode = StartMode::kStartNow);

  template <typename Rep, typename Period>
  Timer(ev::ThreadControl& thread_control, Func on_timer_func,
        const std::chrono::duration<Rep, Period>& call_after,
        StartMode start_mode = StartMode::kStartNow);

  template <typename Rep1, typename Period1, typename Rep2, typename Period2>
  Timer(Func on_timer_func,
        const std::chrono::duration<Rep1, Period1>& first_call_after,
        const std::chrono::duration<Rep2, Period2>& repeat_every,
        StartMode start_mode = StartMode::kStartNow);

  template <typename Rep, typename Period>
  Timer(Func on_timer_func,
        const std::chrono::duration<Rep, Period>& call_after,
        StartMode start_mode = StartMode::kStartNow);

  Timer(const Timer&) = delete;
  Timer& operator=(const Timer&) = delete;

  void Start() { timer_event_.Start(); }
  void Stop() { timer_event_.Stop(); }

 private:
  impl::TimerEvent timer_event_;
};

template <typename Rep1, typename Period1, typename Rep2, typename Period2>
Timer::Timer(ev::ThreadControl& thread_control, Func on_timer_func,
             const std::chrono::duration<Rep1, Period1>& first_call_after,
             const std::chrono::duration<Rep2, Period2>& repeat_every,
             StartMode start_mode)
    : timer_event_(
          thread_control,
          [
            task_processor = &current_task::GetTaskProcessor(),
            on_timer_func_ptr = std::make_shared<Func>(std::move(on_timer_func))
          ]() {
            Async(*task_processor,
                  [on_timer_func_ptr]() { (*on_timer_func_ptr)(); });
          },
          first_call_after, repeat_every, start_mode) {}

template <typename Rep, typename Period>
Timer::Timer(ev::ThreadControl& thread_control, Func on_timer_func,
             const std::chrono::duration<Rep, Period>& call_after,
             StartMode start_mode)
    : Timer(thread_control, std::move(on_timer_func), call_after,
            std::chrono::seconds(0), start_mode) {}

template <typename Rep1, typename Period1, typename Rep2, typename Period2>
Timer::Timer(Func on_timer_func,
             const std::chrono::duration<Rep1, Period1>& first_call_after,
             const std::chrono::duration<Rep2, Period2>& repeat_every,
             StartMode start_mode)
    : Timer(current_task::GetEventThread(), std::move(on_timer_func),
            first_call_after, repeat_every, start_mode) {}

template <typename Rep, typename Period>
Timer::Timer(Func on_timer_func,
             const std::chrono::duration<Rep, Period>& call_after,
             StartMode start_mode)
    : Timer(current_task::GetEventThread(), std::move(on_timer_func),
            call_after, std::chrono::seconds(0), start_mode) {}

}  // namespace engine
