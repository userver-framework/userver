#pragma once

#include <functional>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <engine/deadline.hpp>
#include <engine/ev/thread_control.hpp>

namespace engine::ev {

// Timer is not thread-safe, IOW you cannot call Start() and Stop() in parallel.
class Timer final {
 public:
  // calls on_timer_func() in event loop
  using Func = std::function<void()>;

  Timer() noexcept;
  ~Timer();

  Timer(const Timer&) = delete;
  Timer& operator=(const Timer&) = delete;
  Timer(Timer&&) noexcept = default;
  Timer& operator=(Timer&&) noexcept = default;

  /// Asynchronously starts the timer.
  ///
  /// Postconditions: IsValid() == true
  void Start(ev::ThreadControl& thread_control, Func on_timer_func,
             Deadline deadline);

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
  void Restart(Func on_timer_func, Deadline deadline);

  bool IsValid() const noexcept;
  explicit operator bool() const noexcept { return IsValid(); }

 private:
  class TimerImpl;

  boost::intrusive_ptr<TimerImpl> impl_;
};

}  // namespace engine::ev
