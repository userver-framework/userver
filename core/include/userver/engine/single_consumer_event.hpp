#pragma once

/// @file userver/engine/single_consumer_event.hpp
/// @brief @copybrief engine::SingleConsumerEvent

#include <atomic>
#include <chrono>

#include <userver/engine/deadline.hpp>
#include <userver/engine/impl/wait_list_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @ingroup userver_concurrency
///
/// @brief A multiple-producers, single-consumer event
class SingleConsumerEvent final {
 public:
  struct NoAutoReset final {};

  /// Creates an event that resets automatically on retrieval.
  SingleConsumerEvent() noexcept;

  /// Creates an event that does not reset automatically.
  explicit SingleConsumerEvent(NoAutoReset) noexcept;

  SingleConsumerEvent(const SingleConsumerEvent&) = delete;
  SingleConsumerEvent(SingleConsumerEvent&&) = delete;
  SingleConsumerEvent& operator=(const SingleConsumerEvent&) = delete;
  SingleConsumerEvent& operator=(SingleConsumerEvent&&) = delete;
  ~SingleConsumerEvent();

  /// @return whether this event resets automatically on retrieval
  bool IsAutoReset() const noexcept;

  /// @brief Waits until the event is in a signaled state.
  ///
  /// If the event is auto-resetting, clears the signal flag upon waking up. If
  /// already in a signaled state, does the same without sleeping.
  ///
  /// If we the waiting failed (the event did not signal), because the optional
  /// deadline is expired or the current task is cancelled, returns `false`.
  ///
  /// @return whether the event signaled
  [[nodiscard]] bool WaitForEvent();

  /// @overload bool WaitForEvent()
  template <typename Clock, typename Duration>
  [[nodiscard]] bool WaitForEventFor(std::chrono::duration<Clock, Duration>);

  /// @overload bool WaitForEvent()
  template <typename Clock, typename Duration>
  [[nodiscard]] bool WaitForEventUntil(
      std::chrono::time_point<Clock, Duration>);

  /// @overload bool WaitForEvent()
  [[nodiscard]] bool WaitForEventUntil(Deadline);

  /// @brief Works like `std::condition_variable::wait_until`. Waits until
  /// @a stop_waiting becomes `true`, and we are notified via @ref Send.
  ///
  /// If @a stop_waiting` is already `true`, returns right away.
  ///
  /// Unlike `std::condition_variable` and engine::ConditionVariable, there are
  /// no locks around the state watched by @a stop_waiting, so that state must
  /// be atomic. `std::memory_order_relaxed` is OK inside @a stop_waiting and
  /// inside the notifiers as long as it does not mess up their logic.
  ///
  /// **Example.** Suppose we want to wait until a counter is even, then grab
  /// it.
  ///
  /// Initialization:
  /// @snippet engine/single_consumer_event_test.cpp  CV init
  ///
  /// Notifier side:
  /// @snippet engine/single_consumer_event_test.cpp  CV notifier
  ///
  /// Waiter side:
  /// @snippet engine/single_consumer_event_test.cpp  CV waiter
  template <typename Predicate>
  [[nodiscard]] bool WaitUntil(Deadline, Predicate stop_waiting);

  /// Resets the signal flag. Guarantees at least 'acquire' and 'release'
  /// memory ordering. Must only be called by the waiting task.
  void Reset() noexcept;

  /// Sets the signal flag and wakes a coroutine that waits on it (if any).
  /// If the signal flag is already set, does nothing.
  ///
  /// The waiter is allowed to destroy the SingleConsumerEvent immediately
  /// after exiting WaitForEvent, ONLY IF the wait succeeded. Otherwise
  /// a concurrent task may call Send on a destroyed SingleConsumerEvent.
  /// Here is an example of this situation:
  /// @snippet engine/single_consumer_event_test.cpp  Wait and destroy
  void Send();

  /// Returns `true` iff already signaled. Never resets the signal.
  [[nodiscard]] bool IsReady() const noexcept;

 private:
  class EventWaitStrategy;

  bool GetIsSignaled() noexcept;

  void CheckIsAutoResetForWaitPredicate();

  impl::FastPimplWaitListLight waiters_;
  const bool is_auto_reset_{true};
};

template <typename Clock, typename Duration>
bool SingleConsumerEvent::WaitForEventFor(
    std::chrono::duration<Clock, Duration> duration) {
  return WaitForEventUntil(Deadline::FromDuration(duration));
}

template <typename Clock, typename Duration>
bool SingleConsumerEvent::WaitForEventUntil(
    std::chrono::time_point<Clock, Duration> time_point) {
  return WaitForEventUntil(Deadline::FromTimePoint(time_point));
}

template <typename Predicate>
bool SingleConsumerEvent::WaitUntil(Deadline deadline, Predicate stop_waiting) {
  CheckIsAutoResetForWaitPredicate();

  // If the state, according to what we've been previously notified of via
  // 'Send', is OK, then return right away. Fresh state updates can also
  // leak to us here, but we should not rely on it.
  while (!stop_waiting()) {
    // Wait until we are allowed to make progress.
    // On the first such wait, we may discover a signal from the state that
    // has already leaked to us previously (as described above).
    //
    // We may also receive false signals from cases when we are allowed
    // and unallowed to make progress in a rapid sequence, or when the notifier
    // thinks that we might be happy with the state, but we aren't.
    if (!WaitForEventUntil(deadline)) {
      return false;
    }
  }

  return true;
}

}  // namespace engine

USERVER_NAMESPACE_END
