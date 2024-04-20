#pragma once

/// @file userver/engine/single_use_event.hpp
/// @brief @copybrief engine::SingleUseEvent

#include <userver/engine/deadline.hpp>
#include <userver/engine/future_status.hpp>
#include <userver/engine/impl/context_accessor.hpp>
#include <userver/engine/impl/wait_list_fwd.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @ingroup userver_concurrency
///
/// @brief A single-producer, single-consumer event.
///
/// Once the producer sends the event, it remains in the signaled state forever.
///
/// SingleUseEvent can be used as a faster non-allocating alternative
/// to engine::Future. However, it is more low-level and error-prone, see below.
///
/// Compatible with engine::WaitAny and friends.
///
/// ## Destroying a SingleUseEvent after waking up
///
/// The waiting coroutine is allowed to immediately destroy the `SingleUseEvent`
/// after waking up; it will not stop a concurrent `Send` from completing
/// correctly. This is contrary to the properties of other userver
/// synchronization primitives, like engine::Mutex.
///
/// However, if the wait operation ends in something other than
/// engine::Future::kReady, then it is the responsibility of the waiter
/// to guarantee that it either prevents the oncoming `Send` call or awaits it.
/// One way to force waiting until the `Send` call happens is to use
/// engine::SingleUseEvent::WaitNonCancellable.
///
/// ## Example usage
///
/// @snippet engine/single_use_event_test.cpp  Wait and destroy
///
/// @see @ref scripts/docs/en/userver/synchronization.md
class SingleUseEvent final : private impl::ContextAccessor {
 public:
  SingleUseEvent() noexcept;

  SingleUseEvent(const SingleUseEvent&) = delete;
  SingleUseEvent(SingleUseEvent&&) = delete;
  SingleUseEvent& operator=(const SingleUseEvent&) = delete;
  SingleUseEvent& operator=(SingleUseEvent&&) = delete;
  ~SingleUseEvent();

  /// @brief Waits until the event is in a signaled state.
  ///
  /// @throws engine::WaitInterruptedException if the current task is cancelled
  void Wait();

  /// @brief Waits until the event is in a signaled state, or the deadline
  /// expires, or the current task is cancelled.
  [[nodiscard]] FutureStatus WaitUntil(Deadline);

  /// @brief Waits until the event is in a signaled state, ignoring task
  /// cancellations.
  ///
  /// The waiter coroutine can destroy the `SingleUseEvent` object immediately
  /// after waking up, if necessary.
  void WaitNonCancellable() noexcept;

  /// Sets the signal flag and wakes a coroutine that waits on it, if any.
  /// `Send` must not be called again.
  void Send() noexcept;

  /// Returns true iff already signaled.
  [[nodiscard]] bool IsReady() const noexcept override;

  /// @cond
  // For internal use only.
  impl::ContextAccessor* TryGetContextAccessor() noexcept { return this; }
  /// @endcond

 private:
  friend class impl::FutureWaitStrategy<SingleUseEvent>;

  impl::EarlyWakeup TryAppendWaiter(impl::TaskContext& waiter) override;
  void RemoveWaiter(impl::TaskContext& waiter) noexcept override;
  void RethrowErrorResult() const override;
  void AfterWait() noexcept override;

  impl::FastPimplWaitListLight waiters_;
};

}  // namespace engine

USERVER_NAMESPACE_END
