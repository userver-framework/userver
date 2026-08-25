#pragma once

/// @file userver/engine/single_consumer_event.hpp
/// @brief @copybrief engine::SingleConsumerEvent

#include <chrono>

#include <userver/engine/awaitable.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/future_status.hpp>
#include <userver/utils/fast_pimpl.hpp>

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
    /// If the waiting failed (the event did not signal), because the optional
    /// deadline is expired or the current task is cancelled, returns `false`.
    ///
    /// @return whether the event signaled
    [[nodiscard]] bool WaitForEvent();

    /// @overload bool WaitForEvent()
    template <typename Clock, typename Duration>
    [[nodiscard]] bool WaitForEventFor(std::chrono::duration<Clock, Duration>);

    /// @overload bool WaitForEvent()
    template <typename Clock, typename Duration>
    [[nodiscard]] bool WaitForEventUntil(std::chrono::time_point<Clock, Duration>);

    /// @overload bool WaitForEvent()
    [[nodiscard]] bool WaitForEventUntil(Deadline);

    /// @brief Waits until the event is in a signaled state, same as @ref
    /// engine::SingleConsumerEvent::WaitForEventUntil, but gives the precise reason of a failure instead of just
    /// `false`.
    ///
    /// If the event is auto-resetting, clears the signal flag upon waking up. If already in a signaled state,
    /// does the same without sleeping.
    ///
    /// @return `FutureStatus::kReady` if the event signaled, `FutureStatus::kCancelled` if the current task was
    /// cancelled, `FutureStatus::kTimeout` if the deadline was reached.
    [[nodiscard]] FutureStatus WaitUntil(Deadline deadline);

    /// @brief Works like `std::condition_variable::wait_until`. Waits until
    /// @a stop_waiting becomes `true`, and we are notified via `Send`.
    ///
    /// If @a stop_waiting is already `true`, returns right away.
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
    /// @snippet core/src/engine/single_consumer_event_test.cpp  CV init
    ///
    /// Notifier side:
    /// @snippet core/src/engine/single_consumer_event_test.cpp  CV notifier
    ///
    /// Waiter side:
    /// @snippet core/src/engine/single_consumer_event_test.cpp  CV waiter
    ///
    /// @return `FutureStatus::kReady` if @a stop_waiting became `true`, `FutureStatus::kCancelled` if the current
    /// task was cancelled, `FutureStatus::kTimeout` if the deadline was reached.
    template <typename Predicate>
    [[nodiscard]] FutureStatus WaitUntil(Deadline, Predicate stop_waiting);

    /// Resets the signal flag, if there is any existing event. Guarantees at least 'acquire' and 'release'
    /// memory ordering. Must only be called by the waiting task.
    void Reset() noexcept;

    /// Sets the signal flag and wakes a task that waits on it (if any).
    /// If the signal flag is already set, does nothing.
    ///
    /// The waiter is allowed to destroy the SingleConsumerEvent immediately
    /// after exiting WaitForEvent, ONLY IF the wait succeeded. Otherwise
    /// a concurrent task may call Send on a destroyed SingleConsumerEvent.
    /// Here is an example of this situation:
    /// @snippet core/src/engine/single_consumer_event_test.cpp  Wait and destroy
    ///
    /// You can safely invoke Send from outside a coroutine.
    void Send();

    /// Returns `true` iff already signaled. Never resets the signal.
    [[nodiscard]] bool IsReady() const noexcept;

    /// @brief Satisfies @ref engine::Awaitable, for use with @ref engine::WaitAnyContext and friends.
    ///
    /// @note When using `SingleConsumerEvent` as a condition variable, beware of spurious wakeups.
    /// The awaitable signals completion as soon as @ref Send is called regardless of possible semantic restrictions
    /// of the predicate in @ref WaitUntil.
    ///
    /// @warning Only available for @ref NoAutoReset case.
    AwaitableToken GetAwaitableToken();

private:
    struct Impl;

    bool GetIsSignaled() noexcept;

    utils::FastPimpl<Impl, 32, 16> impl_;
};

template <typename Clock, typename Duration>
bool SingleConsumerEvent::WaitForEventFor(std::chrono::duration<Clock, Duration> duration) {
    return WaitForEventUntil(Deadline::FromDuration(duration));
}

template <typename Clock, typename Duration>
bool SingleConsumerEvent::WaitForEventUntil(std::chrono::time_point<Clock, Duration> time_point) {
    return WaitForEventUntil(Deadline::FromTimePoint(time_point));
}

template <typename Predicate>
FutureStatus SingleConsumerEvent::WaitUntil(Deadline deadline, Predicate stop_waiting) {
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
        if (const auto status = WaitUntil(deadline); status != FutureStatus::kReady) {
            return status;
        }

        if (!IsAutoReset()) {
            // Reset guarantees `std::memory_order_acquire` on the signal, so
            // if we reset any additional signals here, then the predicate will
            // see the associated data updates.
            Reset();
        }
    }

    return FutureStatus::kReady;
}

}  // namespace engine

USERVER_NAMESPACE_END
