#pragma once

/// @file userver/engine/single_use_event.hpp
/// @brief @copybrief engine::SingleUseEvent

#include <atomic>
#include <cstdint>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @ingroup userver_concurrency
///
/// @brief A single-producer, single-consumer event
///
/// Must not be awaited or signaled multiple times in the same waiting session.
///
/// The main advantage of `SingleUseEvent` over `SingleConsumerEvent` is that
/// the waiting coroutine is allowed to immediately destroy the `SingleUseEvent`
/// after waking up; it will not stop a concurrent `Send` from completing
/// correctly.
///
/// Timeouts and cancellations are not supported. Only a concurrent call to
/// `Send` can wake up the waiter. This is necessary for the waiter not to
/// destroy the `SingleUseEvent` unexpectedly for the sender.
///
/// ## Example usage:
///
/// @snippet engine/single_use_event.cpp  Sample engine::SingleUseEvent usage
///
/// @see @ref md_en_userver_synchronization
class SingleUseEvent final {
 public:
  constexpr SingleUseEvent() noexcept : state_(0) {}
  ~SingleUseEvent();

  SingleUseEvent(const SingleUseEvent&) = delete;
  SingleUseEvent(SingleUseEvent&&) = delete;
  SingleUseEvent& operator=(const SingleUseEvent&) = delete;
  SingleUseEvent& operator=(SingleUseEvent&&) = delete;

  /// @brief Waits until the event is in a signaled state
  ///
  /// The event then remains in a signaled state, it does not reset
  /// automatically.
  ///
  /// The waiter coroutine can destroy the `SingleUseEvent` object immediately
  /// after waking up, if necessary.
  void WaitNonCancellable() noexcept;

  /// Sets the signal flag and wakes a coroutine that waits on it, if any.
  /// `Send` must not be called again without `Reset`.
  void Send() noexcept;

  /// Resets the signal flag. Can be called after `WaitNonCancellable` returns
  /// if necessary to reuse the `SingleUseEvent` for another waiting session.
  void Reset() noexcept;

  /// Returns true iff already signaled. Never resets the signal.
  bool IsReady() const noexcept;

 private:
  std::atomic<std::uintptr_t> state_;
};

}  // namespace engine

USERVER_NAMESPACE_END
