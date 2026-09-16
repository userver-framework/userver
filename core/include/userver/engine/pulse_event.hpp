#pragma once

/// @file userver/engine/pulse_event.hpp
/// @brief @copybrief engine::PulseEvent

#include <atomic>
#include <cstdint>

#include <userver/compiler/impl/lifetime.hpp>
#include <userver/engine/awaitable.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/future_status.hpp>
#include <userver/engine/impl/context_accessor.hpp>
#include <userver/engine/impl/wait_list_fwd.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

class PulseEvent;

/// @brief A one-shot subscription from @ref PulseEvent::Subscribe.
///
/// Satisfies @ref engine::Awaitable, for use with @ref engine::WaitAnyContext and friends.
///
/// @note For simple "subscribe to updates" cases a callback API is often more convenient:
/// @ref concurrent::AsyncEventChannel.
///
/// **Example.** A parent task publishes two atomic config fields and wakes all consumers. Each consumer
/// assembles a `Config` and passes it to `ApplyConfig`.
///
/// Initialization:
/// @snippet core/src/engine/pulse_event_test.cpp  WaitUntil subscription init
///
/// Publisher side:
/// @snippet core/src/engine/pulse_event_test.cpp  WaitUntil subscription notifier
///
/// Consumer side:
/// @snippet core/src/engine/pulse_event_test.cpp  WaitUntil subscription waiter
class PulseEventSubscription final : private impl::AwaitableBase {
public:
    PulseEventSubscription() noexcept;
    PulseEventSubscription(const PulseEventSubscription&) = delete;
    PulseEventSubscription(PulseEventSubscription&& other) noexcept;
    PulseEventSubscription& operator=(const PulseEventSubscription&) = delete;
    PulseEventSubscription& operator=(PulseEventSubscription&& other) noexcept;
    ~PulseEventSubscription();

    /// @return `true` if this object was issued by @ref PulseEvent::Subscribe
    [[nodiscard]] bool IsValid() const noexcept;

    /// @return `true` if a @ref PulseEvent::Send after this subscription has already happened
    [[nodiscard]] bool IsReady() const noexcept override;

    /// @brief Waits until a concurrent @ref PulseEvent::Send satisfies this subscription, or the deadline expires, or
    /// the current task is cancelled.
    [[nodiscard]] FutureStatus WaitUntil(Deadline);

    /// Satisfies @ref engine::Awaitable, for use with @ref engine::WaitAnyContext and friends.
    AwaitableToken GetAwaitableToken() noexcept USERVER_IMPL_LIFETIME_BOUND;

private:
    friend class PulseEvent;

    using Ticket = std::uint64_t;

    PulseEventSubscription(PulseEvent* event, Ticket ticket) noexcept;

    void TryAppendAwaiter(impl::AwaiterPtr& awaiter, std::uintptr_t context) override;
    impl::AwaiterPtr RemoveAwaiter(impl::Awaiter& awaiter, std::uintptr_t context) noexcept override;

    PulseEvent* event_{nullptr};
    Ticket ticket_{};
};

/// @ingroup userver_concurrency
///
/// @brief A multiple-producers, multiple-consumers notification.
///
/// @ref Subscribe issues a one-shot @ref PulseEventSubscription. Any @ref Send after that satisfies it; a @ref Send
/// that happened before @ref Subscribe does not count. After that first @ref Send, the subscription stays ready;
/// call @ref Subscribe again to wait for later signals.
///
/// This is unlike @ref engine::MultiConsumerEvent, which latches a single global signal forever after the first
/// @ref engine::MultiConsumerEvent::Send "Send" (equivalent to @ref engine::SingleConsumerEvent::NoAutoReset).
///
/// @ref PulseEventSubscription is compatible with @ref engine::WaitAny and friends. `PulseEvent` itself is not
/// an awaitable: wait on a subscription from @ref Subscribe.
///
/// @see @ref scripts/docs/en/userver/synchronization.md "Synchronization primitives (PulseEvent overview and example)"
class PulseEvent final {
    using Ticket = std::uint64_t;

public:
    PulseEvent() noexcept;

    PulseEvent(const PulseEvent&) = delete;
    PulseEvent(PulseEvent&&) = delete;
    PulseEvent& operator=(const PulseEvent&) = delete;
    PulseEvent& operator=(PulseEvent&&) = delete;
    ~PulseEvent();

    /// @brief Issues a one-shot subscription that later @ref Send calls can satisfy.
    [[nodiscard]] PulseEventSubscription Subscribe() noexcept;

    /// @brief Works like `std::condition_variable::wait_until`. Waits until @a stop_waiting becomes `true`, and we are
    /// notified via `Send`.
    ///
    /// If @a stop_waiting is already `true`, returns right away.
    ///
    /// Unlike `std::condition_variable` and engine::ConditionVariable, there are no locks around the state watched by
    /// @a stop_waiting, so that state must be atomic. `std::memory_order_relaxed` is OK inside @a stop_waiting and
    /// inside the notifiers as long as it does not mess up their logic.
    ///
    /// For a full example, see @ref engine_pulse_event.
    ///
    /// @return `FutureStatus::kReady` if @a stop_waiting became `true`, `FutureStatus::kCancelled` if the current task
    /// was cancelled, `FutureStatus::kTimeout` if the deadline was reached.
    template <typename Predicate>
    [[nodiscard]] FutureStatus WaitUntil(Deadline, Predicate stop_waiting);

    /// Wakes all tasks that currently wait on this event, if any. Also satisfies every subscription already issued, so
    /// a @ref Send that races with @ref Subscribe is not lost.
    ///
    /// You can safely invoke Send from outside a coroutine.
    void Send() noexcept;

private:
    friend class PulseEventSubscription;

    std::atomic<Ticket> generation_{0};
    impl::FastPimplWaitList awaiters_;
};

template <typename Predicate>
FutureStatus PulseEvent::WaitUntil(Deadline deadline, Predicate stop_waiting) {
    while (true) {
        auto subscription = Subscribe();

        if (stop_waiting()) {
            return FutureStatus::kReady;
        }

        if (const auto status = subscription.WaitUntil(deadline); status != FutureStatus::kReady) {
            return status;
        }
    }
}

}  // namespace engine

USERVER_NAMESPACE_END
