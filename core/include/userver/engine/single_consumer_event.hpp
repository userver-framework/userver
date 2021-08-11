#pragma once

/// @file userver/engine/single_consumer_event.hpp
/// @brief @copybrief engine::SingleConsumerEvent

#include <atomic>
#include <memory>

#include <userver/engine/deadline.hpp>
#include <userver/engine/impl/wait_list_fwd.hpp>

namespace engine {

/// Event for a single awaiter, multiple signal coroutines
class SingleConsumerEvent final {
 public:
  struct NoAutoReset {};

  /// Creates an event for a single consumer. It will reset automatically on
  /// retrieval.
  SingleConsumerEvent();

  /// Creates an event for a single consumer that does not reset automatically.
  explicit SingleConsumerEvent(NoAutoReset);

  ~SingleConsumerEvent();

  SingleConsumerEvent(const SingleConsumerEvent&) = delete;
  SingleConsumerEvent(SingleConsumerEvent&&) = delete;
  SingleConsumerEvent& operator=(const SingleConsumerEvent&) = delete;
  SingleConsumerEvent& operator=(SingleConsumerEvent&&) = delete;

  /// @return whether this event resets automatically on retrieval
  bool IsAutoReset() const;

  /// Waits until the event is in a signaled state, then atomically
  /// wakes up and clears the signal flag if the event is auto-resetting. If
  /// already in a signaled state, does the same without sleeping.
  /// @returns whether the event signaled (otherwise task could've been
  /// cancelled)
  [[nodiscard]] bool WaitForEvent();

  template <typename Clock, typename Duration>
  [[nodiscard]] bool WaitForEventFor(std::chrono::duration<Clock, Duration>);

  template <typename Clock, typename Duration>
  [[nodiscard]] bool WaitForEventUntil(
      std::chrono::time_point<Clock, Duration>);

  [[nodiscard]] bool WaitForEventUntil(Deadline);

  /// Resets the signal flag.
  void Reset();

  /// Sets the signal flag and wakes a coroutine that waits on it (if any).
  /// If the signal flag is already set, does nothing.
  void Send();

 private:
  bool GetIsSignaled();

  impl::FastPimplWaitListLight lock_waiters_;
  const bool is_auto_reset_{true};
  std::atomic<bool> is_signaled_{false};
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
