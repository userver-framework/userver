#pragma once

#include <atomic>
#include <functional>

#include <userver/concurrent/impl/interference_shield.hpp>
#include <userver/concurrent/impl/intrusive_hooks.hpp>
#include <userver/concurrent/impl/intrusive_mpsc_queue.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskContext;

// An MPSC queue where the consumer is spontaneously chosen among producers.
// Also, there is an option to provide a dedicated worker task, which will
// become a permanent producer, until it decides to leave.
class AsyncFlatCombiningQueue final {
public:
    enum class NotificationMode {
        kNotify,
        kDeferred,
    };

    class Consumer;
    using NodeBase = concurrent::impl::SinglyLinkedBaseHook;

    AsyncFlatCombiningQueue();
    ~AsyncFlatCombiningQueue();

    // The queue takes the ownership of the node.
    // If `kNotify` and an async consumer is sleeping, it is woken up.
    // If `kDeferred`, the push is "deferred": a sleeping async consumer is not woken up
    // and is expected to be notified later by some other means.
    Consumer PushAndTryStartConsuming(NodeBase& node, NotificationMode notify) noexcept;

    // Note: only 1 task may wait at a time.
    Consumer WaitAndStartConsuming();

    // Note: only 1 task may wait at a time.
    void WaitWhileEmpty(Consumer& consumer) noexcept;

private:
    template <auto TryStartWaiting>
    class Awaitable;

    enum class NotificationState {
        kWorking,
        kWorkingNotified,
        kSleeping,
    };

    static_assert(std::atomic<NotificationState>::is_always_lock_free);

    // Wakes up the async consumer if it is currently sleeping. Safe to call from
    // any thread, including non-coroutine ones.
    void NotifyAsyncConsumerIfSleeping() noexcept;

    NodeBase* DoTryPop() noexcept;

    bool DoTryStopConsuming() noexcept;

    void NotifyAsyncConsumer() noexcept;

    template <auto TryStartWaiting>
    void Wait() noexcept;

    bool TryStartWaitingWhileEmpty();

    bool TryStartWaitingForConsumer();

    // Tracks whether the async consumer is working or sleeping, decoupling the
    // wakeup decision from the queue contents (see NotifyConsumer / WaitWhileEmpty).
    // While there is no async consumer, the state is permanently kWorkingNotified.
    // Kept first to avoid excessive class padding (it is cache-line aligned).
    concurrent::impl::InterferenceShield<std::atomic<NotificationState>> notification_state_{
        NotificationState::kWorkingNotified
    };

    concurrent::impl::IntrusiveMpscQueueImpl queue_;

    // Whoever detects this node in the back, becomes the consumer.
    concurrent::impl::SinglyLinkedBaseHook consumer_node_;
    // Signals to the current consumer that it should hand over to the async task.
    concurrent::impl::SinglyLinkedBaseHook start_consuming_notifier_node_;

    TaskContext* consuming_task_context_{nullptr};
    bool should_pass_consumer_to_waiter_{false};
    // For diagnosing a multiple-consumers situation.
    std::atomic<bool> has_consumer_{false};
    // For diagnosing a multiple-waiters situation.
    std::atomic<bool> has_waiter_{false};
};

class AsyncFlatCombiningQueue::Consumer final {
public:
    Consumer() noexcept = default;

    Consumer(Consumer&&) noexcept;
    Consumer& operator=(Consumer&&) noexcept;
    ~Consumer();

    bool IsValid() const noexcept { return queue_ != nullptr; }

    // The caller takes the ownership of the node.
    NodeBase* TryPop() noexcept;

    bool TryStopConsuming() noexcept;

    template <typename ConsumerFunc>
    void ConsumeAndStop(ConsumerFunc consumer_func) && noexcept;

private:
    friend class AsyncFlatCombiningQueue;

    explicit Consumer(AsyncFlatCombiningQueue& queue);

    AsyncFlatCombiningQueue* queue_{nullptr};
};

inline auto AsyncFlatCombiningQueue::PushAndTryStartConsuming(NodeBase& node, NotificationMode notify)
    noexcept -> Consumer {
    // Moving this function definition to cpp degrades performance.

    const auto* const prev = queue_.GetBackAndPush(node);

    if (prev == &consumer_node_) {
        // There is no async consumer; the caller becomes the consumer itself.
        return Consumer{*this};
    }

    if (NotificationMode::kNotify == notify) {
        NotifyAsyncConsumerIfSleeping();
    }

    return Consumer{};
}

template <typename ConsumerFunc>
void AsyncFlatCombiningQueue::Consumer::ConsumeAndStop(ConsumerFunc consumer_func) && noexcept {
    static_assert(std::is_nothrow_invocable_v<ConsumerFunc&, NodeBase&>);
    do {
        while (auto* const node = TryPop()) {
            consumer_func(*node);
        }
    } while (!TryStopConsuming());
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
