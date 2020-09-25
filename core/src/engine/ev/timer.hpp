#pragma once

#include <chrono>
#include <functional>
#include <memory>

#include <engine/ev/thread_control.hpp>

namespace engine::ev {

// Timer is not thread-safe, IOW you cannot call Start() and Stop() in parallel.
class Timer final {
 public:
  // calls on_timer_func() in event loop
  using Func = std::function<void()>;

  Timer() noexcept = default;
  ~Timer();

  Timer(const Timer&) = delete;
  Timer& operator=(const Timer&) = delete;
  Timer(Timer&&) noexcept = default;
  Timer& operator=(Timer&&) noexcept = default;

  /// Starts the timer.
  ///
  /// Postconditions: IsValid() == true
  template <typename Rep1, typename Period1, typename Rep2, typename Period2>
  void Start(ev::ThreadControl& thread_control, Func on_timer_func,
             std::chrono::duration<Rep1, Period1> first_call_after,
             std::chrono::duration<Rep2, Period2> repeat_every);

  /// Asynchronously starts the timer.
  ///
  /// Postconditions: IsValid() == true
  template <typename Rep, typename Period>
  void Start(ev::ThreadControl& thread_control, Func on_timer_func,
             std::chrono::duration<Rep, Period> call_after);

  /// Asynchronously stops and releases all the resources associated with the
  /// timer. Does nothing for a IsValid() == false timer.
  ///
  /// Postconditions: IsValid() == false
  void Stop() noexcept;

  /// Restarts a Rrnning timer with specified params. More efficient than
  /// calling Stop() + Start().
  ///
  /// Preconditions: IsValid() == true
  /// Postconditions: IsValid() == true
  template <typename Rep1, typename Period1, typename Rep2, typename Period2>
  void Restart(Func on_timer_func,
               std::chrono::duration<Rep1, Period1> first_call_after,
               std::chrono::duration<Rep2, Period2> repeat_every);

  template <typename Rep1, typename Period1>
  void Restart(Func on_timer_func,
               std::chrono::duration<Rep1, Period1> first_call_after);

  bool IsValid() const noexcept;
  explicit operator bool() const noexcept { return IsValid(); }

 private:
  class TimerImpl;

  void Start(ev::ThreadControl& thread_control, Func on_timer_func,
             double first_call_after, double repeat_every);

  void Restart(Func on_timer_func, double first_call_after,
               double repeat_every);

  std::shared_ptr<TimerImpl> impl_;
};

template <typename Rep1, typename Period1, typename Rep2, typename Period2>
void Timer::Start(ev::ThreadControl& thread_control, Func on_timer_func,
                  std::chrono::duration<Rep1, Period1> first_call_after,
                  std::chrono::duration<Rep2, Period2> repeat_every) {
  using std::chrono::duration_cast;
  Start(thread_control, std::move(on_timer_func),
        duration_cast<std::chrono::duration<double>>(first_call_after).count(),
        duration_cast<std::chrono::duration<double>>(repeat_every).count());
}

template <typename Rep, typename Period>
void Timer::Start(ev::ThreadControl& thread_control, Func on_timer_func,
                  std::chrono::duration<Rep, Period> call_after) {
  using std::chrono::duration_cast;
  Start(thread_control, std::move(on_timer_func),
        duration_cast<std::chrono::duration<double>>(call_after).count(), 0.0);
}

template <typename Rep1, typename Period1, typename Rep2, typename Period2>
void Timer::Restart(Func on_timer_func,
                    std::chrono::duration<Rep1, Period1> first_call_after,
                    std::chrono::duration<Rep2, Period2> repeat_every) {
  using std::chrono::duration_cast;
  Restart(
      std::move(on_timer_func),
      duration_cast<std::chrono::duration<double>>(first_call_after).count(),
      duration_cast<std::chrono::duration<double>>(repeat_every).count());
}

template <typename Rep1, typename Period1>
void Timer::Restart(Func on_timer_func,
                    std::chrono::duration<Rep1, Period1> first_call_after) {
  using std::chrono::duration_cast;
  Restart(
      std::move(on_timer_func),
      duration_cast<std::chrono::duration<double>>(first_call_after).count(),
      0.0);
}

}  // namespace engine::ev
