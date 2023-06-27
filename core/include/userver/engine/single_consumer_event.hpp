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

  /// Resets the signal flag. Guarantees at least 'acquire' and 'release' memory
  /// ordering. Must only be called by the waiting task.
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
  bool IsReady() const noexcept;

 private:
  class EventWaitStrategy;

  bool GetIsSignaled() noexcept;

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

}  // namespace engine

USERVER_NAMESPACE_END
