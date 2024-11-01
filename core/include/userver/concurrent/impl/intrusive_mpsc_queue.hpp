#pragma once

#include <atomic>

#include <userver/concurrent/impl/interference_shield.hpp>
#include <userver/concurrent/impl/intrusive_hooks.hpp>
#include <userver/utils/not_null.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

// A minimalistic multiple-producer, single-consumer concurrent queue.
//
// Intrusive, ABA-free, linearizable.
// Nodes may be freed immediately after TryPop.
//
// The queue is "slightly blocking", but on practice blocking happens rarely.
// This design leads to a better performance when compared to similar
// lock-free queues.
//
// When intrusive-ness and linearizability are not required, Moodycamel queue
// yields better performance.
//
// Based on Dmitry Vykov's MPSC queue:
// https://www.1024cores.net/home/lock-free-algorithms/queues/intrusive-mpsc-node-based-queue
class IntrusiveMpscQueueImpl final {
public:
    using NodePtr = SinglyLinkedBaseHook*;
    using NodeRef = utils::NotNull<NodePtr>;

    constexpr IntrusiveMpscQueueImpl() = default;

    IntrusiveMpscQueueImpl(IntrusiveMpscQueueImpl&&) = delete;
    IntrusiveMpscQueueImpl& operator=(IntrusiveMpscQueueImpl&&) = delete;
    ~IntrusiveMpscQueueImpl() = default;

    // Can be called from multiple threads concurrently.
    void Push(NodeRef node) noexcept;

    // Atomically push a node and get the newest node pushed before us, if any.
    // Warning: to use the result, the user needs to somehow guarantee that it
    // will not be deleted by the consumer.
    // Can be called from multiple threads concurrently.
    NodePtr GetBackAndPush(NodeRef node) noexcept;

    // Push a node if the queue is empty.
    // Can be called from multiple threads concurrently.
    [[nodiscard]] bool PushIfEmpty(NodeRef node) noexcept;

    // Returns the oldest pushed node, or `nullptr` if the queue is logically
    // empty. Momentarily spins if necessary for a concurrent Push to complete.
    // Can only be called from one thread at a time.
    NodePtr TryPopBlocking() noexcept;

    // Returns the oldest pushed not, or `nullptr` if the queue "seems to be"
    // empty. Can only be called from one thread at a time.
    // Unlike TryPopBlocking, never blocks, but this comes at a cost: it might
    // not return an item that has been completely pushed (happens-before).
    //
    // The exact semantics of the ordering are as follows.
    // Items are pushed in a flat-combining manner: if two items are being
    // pushed concurrently, then one producer is randomly chosen to be
    // responsible for pushing both items, and the other producer walks away,
    // and for them the push operation is essentially pushed asynchronously.
    //
    // If the producers always notify the consumer after pushing,
    // then TryPopWeak is enough: the consumer will be notified of all
    // the pushed items by some of the producers.
    //
    // If for some items the consumer is not notified, and for some "urgent"
    // items it is notified, then it's not a good idea to use TryPopWeak,
    // because an "urgent" item may not yet be pushed completely
    // upon the notification.
    NodePtr TryPopWeak() noexcept;

private:
    enum class PopMode {
        kRarelyBlocking,
        kWeak,
    };

    static std::atomic<NodePtr>& GetNext(NodeRef node) noexcept;

    NodePtr DoTryPop(PopMode) noexcept;

    // This node is put into the queue when it would otherwise be empty.
    SinglyLinkedBaseHook stub_;
    // Points to the oldest node not yet popped by the consumer,
    // a.k.a. the beginning of the node list.
    // tail_ is only needed for consumer's bookkeeping.
    NodeRef tail_{&stub_};
    // For checking the single-consumer invariant
    std::atomic<bool> is_consuming_{false};
    // Points to the newest added node, a.k.a. the end of the node list.
    InterferenceShield<std::atomic<NodeRef>> head_{NodeRef{&stub_}};
};

template <typename T>
class IntrusiveMpscQueue final {
    static_assert(std::is_base_of_v<SinglyLinkedBaseHook, T>);

public:
    constexpr IntrusiveMpscQueue() = default;

    void Push(T& node) noexcept { impl_.Push(IntrusiveMpscQueueImpl::NodeRef{&node}); }

    T* TryPopBlocking() noexcept { return static_cast<T*>(impl_.TryPopBlocking()); }

    T* TryPopWeak() noexcept { return static_cast<T*>(impl_.TryPopWeak()); }

private:
    IntrusiveMpscQueueImpl impl_;
};

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
