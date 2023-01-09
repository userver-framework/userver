#pragma once

#include <atomic>

#include <concurrent/impl/intrusive_hooks.hpp>
#include <concurrent/impl/tagged_ptr.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

/// @brief A minimalistic multiple-producer, single-consumer concurrent queue.
///
/// Does not scale for high contention: it is expected that on average O(1)
/// threads will push into one of these queues.
///
/// Intrusive, ABA-free, lock-free. Nodes may be freed immediately after TryPop.
///
/// The algorithm:
/// - producers push nodes into the "incoming" stack
/// - consumer first tries to pop from the "reversed" stack
/// - if empty, consumer then takes "incoming" and reverts it into "reversed"
template <typename T, typename HookExtractor>
class IntrusiveMpscQueue final {
 public:
  constexpr IntrusiveMpscQueue() {
    static_assert(std::is_empty_v<HookExtractor>);
    static_assert(
        std::is_invocable_r_v<SinglyLinkedHook<T>&, HookExtractor, T&>);
  }

  void Push(T& node) noexcept {
    UASSERT(GetNext(node) == nullptr);

    T* next = incoming_stack_.load(std::memory_order_relaxed);
    while (true) {
      GetNext(node).store(next, std::memory_order_relaxed);
      // 'acquire' is needed to carry dependency of the previous nodes to the
      // consumer, which will only 'acquire' the stack head.
      if (incoming_stack_.compare_exchange_weak(next, &node,
                                                std::memory_order_acq_rel,
                                                std::memory_order_relaxed)) {
        break;
      }
    }
  }

  T* TryPop() noexcept {
    if (T* const node = reversed_stack_) {
      reversed_stack_ = ExtractTailRelaxed(*node);
      return node;
    }

    if (incoming_stack_.load(std::memory_order_relaxed) == nullptr) {
      return nullptr;
    }

    T* const incoming =
        incoming_stack_.exchange(nullptr, std::memory_order_acquire);
    UASSERT(incoming);

    T* const reversed = ReverseStack(incoming);
    reversed_stack_ = ExtractTailRelaxed(*reversed);
    return reversed;
  }

  T* Peek() noexcept {
    if (T* const node = reversed_stack_) {
      return node;
    }

    if (incoming_stack_.load(std::memory_order_relaxed) == nullptr) {
      return nullptr;
    }

    T* const incoming =
        incoming_stack_.exchange(nullptr, std::memory_order_acquire);
    UASSERT(incoming);

    T* const reversed = ReverseStack(incoming);
    reversed_stack_ = reversed;
    return reversed;
  }

  void PopPeeked(T& node) noexcept {
    UASSERT(reversed_stack_ == node);
    reversed_stack_ = ExtractTailRelaxed(node);
  }

 private:
  static std::atomic<T*>& GetNext(T& node) noexcept {
    return static_cast<SinglyLinkedHook<T>&>(HookExtractor{}(node)).next_;
  }

  static T* ReverseStack(T* original) noexcept {
    T* reversed = nullptr;
    while (original != nullptr) {
      T* const next_incoming =
          GetNext(*original).load(std::memory_order_relaxed);
      GetNext(*original).store(reversed, std::memory_order_relaxed);
      reversed = original;
      original = next_incoming;
    }
    return reversed;
  }

  static T* ExtractTailRelaxed(T& node) noexcept {
    T* const tail = GetNext(node).load(std::memory_order_relaxed);
    GetNext(node).store(nullptr, std::memory_order_relaxed);
    return tail;
  }

  std::atomic<T*> incoming_stack_{nullptr};
  T* reversed_stack_{nullptr};
};

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
