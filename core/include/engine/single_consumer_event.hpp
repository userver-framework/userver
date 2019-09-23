#pragma once

/// @file engine/single_consumer_event.hpp
/// @brief @copybrief engine::SingleConsumerEvent

#include <atomic>
#include <memory>

#include <engine/deadline.hpp>

namespace engine {
namespace impl {

class TaskContext;
class WaitListLight;

}  // namespace impl

/// Event for a single awaiter, multiple signal coroutines
class SingleConsumerEvent final {
 public:
  SingleConsumerEvent();
  ~SingleConsumerEvent();

  SingleConsumerEvent(const SingleConsumerEvent&) = delete;
  SingleConsumerEvent(SingleConsumerEvent&&) = delete;
  SingleConsumerEvent& operator=(const SingleConsumerEvent&) = delete;
  SingleConsumerEvent& operator=(SingleConsumerEvent&&) = delete;

  /// Wait until event is in a signalled state, then atomically
  //  wake up and clear signal flag. If already in signal state,
  //  clear the flag and return without sleeping.
  /// @returns whether the event signalled (otherwise task could've been
  //  cancelled)
  [[nodiscard]] bool WaitForEvent();

  template <typename Clock, typename Duration>
  [[nodiscard]] bool WaitForEventFor(std::chrono::duration<Clock, Duration>);

  template <typename Clock, typename Duration>
  [[nodiscard]] bool WaitForEventUntil(
      std::chrono::time_point<Clock, Duration>);

  [[nodiscard]] bool WaitForEventUntil(Deadline);

  /// Set a signal flag and awake a coroutine that waits on it (if any).
  //  If the signal flag is already set, does nothing.
  void Send();

 private:
  std::shared_ptr<impl::WaitListLight> lock_waiters_;
  std::atomic<bool> is_signaled_;
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
