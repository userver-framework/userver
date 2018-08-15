#pragma once

#include <chrono>
#include <functional>
#include <memory>

#include <engine/ev/thread_control.hpp>

namespace engine {
namespace ev {

class Timer {
 public:
  // calls on_timer_func() in event loop
  using Func = std::function<void()>;

  enum class StartMode { kStartNow, kDeferStart };

  Timer();

  template <typename Rep1, typename Period1, typename Rep2, typename Period2>
  Timer(ev::ThreadControl& thread_control, Func on_timer_func,
        const std::chrono::duration<Rep1, Period1>& first_call_after,
        const std::chrono::duration<Rep2, Period2>& repeat_every,
        StartMode start_mode = StartMode::kStartNow);

  template <typename Rep, typename Period>
  Timer(ev::ThreadControl& thread_control, Func on_timer_func,
        const std::chrono::duration<Rep, Period>& call_after,
        StartMode start_mode = StartMode::kStartNow);

  Timer(const Timer&) = delete;
  Timer& operator=(const Timer&) = delete;
  Timer(Timer&&) noexcept = default;
  Timer& operator=(Timer&&) noexcept = default;

  ~Timer();

  bool IsValid() const;
  explicit operator bool() const { return IsValid(); }

  void Start();
  void Stop();

 private:
  class TimerImpl;

  Timer(ev::ThreadControl& thread_control, Func on_timer_func,
        double first_call_after, double repeat_every, StartMode start_mode);

  std::shared_ptr<TimerImpl> impl_;
};

template <typename Rep1, typename Period1, typename Rep2, typename Period2>
Timer::Timer(ev::ThreadControl& thread_control, Func on_timer_func,
             const std::chrono::duration<Rep1, Period1>& first_call_after,
             const std::chrono::duration<Rep2, Period2>& repeat_every,
             StartMode start_mode)
    : Timer(thread_control, std::move(on_timer_func),
            std::chrono::duration_cast<std::chrono::duration<double>>(
                first_call_after)
                .count(),
            std::chrono::duration_cast<std::chrono::duration<double>>(
                repeat_every)
                .count(),
            start_mode) {}

template <typename Rep, typename Period>
Timer::Timer(ev::ThreadControl& thread_control, Func on_timer_func,
             const std::chrono::duration<Rep, Period>& call_after,
             StartMode start_mode)
    : Timer(thread_control, std::move(on_timer_func), call_after,
            std::chrono::seconds(0), start_mode) {}

}  // namespace ev
}  // namespace engine
