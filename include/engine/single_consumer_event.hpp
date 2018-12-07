#pragma once

/// @file engine/single_consumer_event.hpp
/// @brief @copybrief engine::SingleConsumerEvent

#include <atomic>
#include <memory>

namespace engine {
namespace impl {

class TaskContext;
class WaitListLight;

}  // namespace impl

/// Event for a single awaiter, multiple signal coroutines
class SingleConsumerEvent {
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

  /// Set a signal flag and awake a coroutine that waits on it (if any).
  //  If the signal flag is already set, does nothing.
  void Send();

 private:
  std::shared_ptr<impl::WaitListLight> lock_waiters_;
  std::atomic<bool> signaled_;
};

}  // namespace engine
