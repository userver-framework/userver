#pragma once

#include <boost/intrusive_ptr.hpp>

#include <engine/ev/thread_control.hpp>
#include <engine/task/sleep_state.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskContext;

// A timer bound to a specific TaskContext.
// Not thread-safe, IOW you cannot call Start() and Stop() in parallel.
class ContextTimer final {
 public:
  ContextTimer();

  ContextTimer(const ContextTimer&) = delete;
  ContextTimer& operator=(const ContextTimer&) = delete;
  ContextTimer(ContextTimer&&) noexcept = delete;
  ContextTimer& operator=(ContextTimer&&) noexcept = delete;
  ~ContextTimer();

  bool WasStarted() const noexcept;

  /// Asynchronously starts the timer with cancel request.
  /// Prolongs lifetime of the context until Finalize().
  void StartCancel(boost::intrusive_ptr<TaskContext> context,
                   ev::TimerThreadControl& thread_control, Deadline deadline);

  /// Asynchronously starts the timer with wakeup request.
  void StartWakeup(boost::intrusive_ptr<TaskContext> context,
                   ev::TimerThreadControl& thread_control, Deadline deadline,
                   SleepState::Epoch sleep_epoch);

  /// Restarts a running timer with specified params. More efficient than
  /// calling Stop() + Start().
  void RestartCancel(Deadline deadline);

  /// Restarts a running timer with specified params. More efficient than
  /// calling Stop() + Start().
  void RestartWakeup(Deadline deadline, SleepState::Epoch sleep_epoch);

  /// Asynchronously stops the timer and destroys all held resources.
  /// Invalidates the Timer and makes it unable to be restarted,
  /// releases the context.
  /// Does nothing for a WasStarted() == false timer.
  void Finalize() noexcept;

 private:
  class Impl;
  utils::FastPimpl<Impl, 160, 16> impl_;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
