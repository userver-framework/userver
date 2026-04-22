#pragma once

/// @file userver/engine/multi_consumer_event.hpp
/// @brief @copybrief engine::MultiConsumerEvent

#include <atomic>

#include <userver/engine/deadline.hpp>
#include <userver/engine/future_status.hpp>
#include <userver/engine/impl/context_accessor.hpp>
#include <userver/engine/impl/wait_list_fwd.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace impl {

template <typename T>
class FutureWaitStrategy;

}  // namespace impl

/// @ingroup userver_concurrency
///
/// @brief A single-producer, multiple-consumers event.
///
/// Once the producer sends the event, it remains in the signaled state forever.
///
/// Compatible with @ref engine::WaitAny and friends.
///
/// ## Destroying a MultiConsumerEvent after waking up
///
/// Unlike @ref engine::SingleUseEvent, `MultiConsumerEvent` has multiple consumers, thus a consumer is not allowed to
/// destroy `MultiConsumerEvent` instance, instead the producer should be responsible for that.
///
/// @see @ref scripts/docs/en/userver/synchronization.md
class MultiConsumerEvent final : private impl::ContextAccessor {
public:
    MultiConsumerEvent() noexcept;

    MultiConsumerEvent(const MultiConsumerEvent&) = delete;
    MultiConsumerEvent(MultiConsumerEvent&&) = delete;
    MultiConsumerEvent& operator=(const MultiConsumerEvent&) = delete;
    MultiConsumerEvent& operator=(MultiConsumerEvent&&) = delete;
    ~MultiConsumerEvent();

    /// @brief Waits until the event is in a signaled state.
    ///
    /// @throws engine::WaitInterruptedException if the current task is cancelled
    void Wait();

    /// @brief Waits until the event is in a signaled state, or the deadline
    /// expires, or the current task is cancelled.
    [[nodiscard]] FutureStatus WaitUntil(Deadline);

    /// Sets the signal flag and wakes tasks that wait on it, if any.
    /// `Send` must not be called again.
    ///
    /// You can safely invoke Send from outside a coroutine.
    void Send() noexcept;

    /// Returns true if already signaled.
    [[nodiscard]] bool IsReady() const noexcept override;

    /// @cond
    // For internal use only.
    impl::ContextAccessor* TryGetContextAccessor() noexcept { return this; }
    /// @endcond

private:
    friend class impl::FutureWaitStrategy<MultiConsumerEvent>;

    void TryAppendAwaiter(boost::intrusive_ptr<impl::Awaiter>& awaiter, std::uintptr_t context) override;
    void RemoveAwaiter(impl::Awaiter& awaiter, std::uintptr_t context) noexcept override;

    std::atomic<bool> is_ready_{false};
    impl::FastPimplWaitList awaiters_;
};

}  // namespace engine

USERVER_NAMESPACE_END
