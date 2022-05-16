#pragma once

#include <functional>

#include <engine/ev/thread_control.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {
class TaskContext;
}

namespace engine::ev {

// Timer is not thread-safe, IOW you cannot call Start() and Stop() in parallel.
class Timer final {
 public:
  // calls on_timer_func() in event loop
  using Func = std::function<void()>;

  /// `owner` must live for at least as long as the Timer.
  explicit Timer(engine::impl::TaskContext& owner);

  Timer(const Timer&) = delete;
  Timer& operator=(const Timer&) = delete;
  Timer(Timer&&) noexcept = delete;
  Timer& operator=(Timer&&) noexcept = delete;
  ~Timer();

  /// Asynchronously starts the timer.
  ///
  /// Preconditions: IsValid() == false
  /// Postconditions: IsValid() == true
  void Start(ThreadControl thread_control, Func&& on_timer_func,
             Deadline deadline);

  /// Restarts a running timer with specified params. More efficient than
  /// calling Stop() + Start().
  ///
  /// Preconditions: IsValid() == true
  /// Postconditions: IsValid() == true
  void Restart(Func&& on_timer_func, Deadline deadline);

  /// Asynchronously stops the timer. Does not invalidate the timer.
  /// Does nothing for a IsValid() == false timer.
  void Stop() noexcept;

  bool IsValid() const noexcept;

 private:
  class Impl;
  utils::FastPimpl<Impl, 288, 16> impl_;
};

}  // namespace engine::ev

USERVER_NAMESPACE_END
