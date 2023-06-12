#pragma once

#include <atomic>

#include <concurrent/impl/interference_shield.hpp>
#include <concurrent/impl/intrusive_hooks.hpp>
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

  // Returns the oldest pushed node, or `nullptr` if the queue is logically
  // empty. Momentarily spins if necessary for a concurrent Push to complete.
  // Can only be called from one thread at a time.
  NodePtr TryPop() noexcept;

 private:
  static std::atomic<NodePtr>& GetNext(NodeRef node) noexcept;

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

  void Push(T& node) noexcept {
    impl_.Push(IntrusiveMpscQueueImpl::NodeRef{&node});
  }

  T* TryPop() noexcept { return static_cast<T*>(impl_.TryPop()); }

 private:
  IntrusiveMpscQueueImpl impl_;
};

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
