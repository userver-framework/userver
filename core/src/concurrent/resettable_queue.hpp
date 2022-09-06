#pragma once

#include <atomic>
#include <type_traits>

#include <boost/intrusive/slist.hpp>
#include <boost/intrusive/slist_hook.hpp>

#include <concurrent/impl/fast_atomic.hpp>
#include <concurrent/intrusive_walkable_pool.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/flags.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

/// @brief A queue that allows to remove elements from the middle without
/// generating garbage.
///
/// Elements in a regular Michael-Scott queue may be removed by CAS-them away.
/// However, this leaves garbage nodes in the middle of the queue. If pushes are
/// infrequent, elements are only removed from the middle, this may become a
/// memory leak. This modified queue attempts to resolve those issues by reusing
/// those "logically removed" nodes.
///
/// The element type `T` must:
/// 1. be trivially-copyable and have no padding
/// 2.1. `T::MakeInvalid(std::uint32_t invalid_id) -> T`
/// 2.2. `T::IsValid(T value) -> bool`
/// 2.3. `T::GetInvalidId(T value) -> std::uint32_t`
/// 3. be equality-comparable using `operator==`
///
/// The inserted elements must be "reasonably unique" for the queue to work
/// correctly (see the details below).
template <typename T>
class ResettableQueue final {
 public:
  ResettableQueue();

  ResettableQueue(ResettableQueue&&) = delete;
  ResettableQueue& operator=(ResettableQueue&&) = delete;
  ~ResettableQueue();

  class ItemHandle;

  /// Enqueue an element. Returns an `ItemHandle`, which can be used to `Remove`
  /// the element later.
  ItemHandle Push(T value);

  /// Dequeue an element, if the queue is not empty.
  ///
  /// Uniqueness requirement consequences: If a value has been Popped, and the
  /// same value has been Pushed again in parallel with TryPop, then that same,
  /// but unrelated value may be retrieved instead, violating FIFO order and
  /// producing a garbage node.
  bool TryPop(T& value) noexcept;

  /// Remove the originally Pushed value, if it is still there.
  ///
  /// Uniqueness requirement consequences: If a value has been Popped, and the
  /// same value has been Pushed again in parallel with Remove, then that same,
  /// but unrelated value may be removed instead.
  void Remove(ItemHandle&& handle) noexcept;

 private:
  static constexpr T MakeInvalid(std::uint32_t invalid_id) noexcept {
    return T::MakeInvalid(invalid_id);
  }

  static bool IsValid(T value) noexcept { return T::IsValid(value); }

  static std::uint32_t GetInvalidId(T value) noexcept {
    UASSERT(!IsValid(value));
    return T::GetInvalidId(value);
  }

  // The value of the node has been extracted by TryPop. The node is thus at
  // least logically removed. It may still be needed by the queue as a
  // Michael-Scott dummy node.
  inline static const T kRemoved = MakeInvalid(0x10000);

  // The value of the node has been cleared in Remove. The value of the node has
  // still not been extracted by TryPop. The node may thus be reused without a
  // proper push-pop sequence.
  static T MakeVacant(std::uint16_t tag) noexcept {
    return MakeInvalid(std::uint32_t{tag});
  }

  // A subcategory of kRemoved. The value of the node has been cleared in
  // Remove. Then the value of the node has been extracted by TryPop. However,
  // it has not yet been physically removed from the queue, serving as a dummy
  // node.
  inline static const T kVacantDummy = MakeInvalid(0x10001);

  using LocalFreeListHook = boost::intrusive::slist_member_hook<
      boost::intrusive::link_mode<boost::intrusive::normal_link>>;

  struct Node final {
    FastAtomic<T> value{kRemoved};
    std::atomic<TaggedPtr<Node>> next{nullptr};
    std::atomic<std::uint16_t> next_vacant_tag{0};
    IntrusiveStackHook<Node> free_or_vacant_hook{};
    LocalFreeListHook local_free_hook{};
  };

  class FreeList;

  void DoPush(Node& node) noexcept;
  bool DoPopValue(Node& node, T expected) noexcept;
  void DoPopNode(Node& node) noexcept;

  std::atomic<TaggedPtr<Node>> head_{nullptr};
  std::atomic<TaggedPtr<Node>> tail_{nullptr};
  IntrusiveStack<Node, MemberHook<&Node::free_or_vacant_hook>> vacant_list_{};
};

template <typename T>
class ResettableQueue<T>::ItemHandle final {
 public:
  ItemHandle() noexcept = default;

 private:
  friend class ResettableQueue;

  ItemHandle(Node& node, T value) noexcept : node_(&node), value_(value) {}

  Node* node_{nullptr};
  T value_{};
};

template <typename T>
class ResettableQueue<T>::FreeList final {
 public:
  static Node& Acquire() {
    if (Node* const node = local_.TryPop()) return *node;
    if (Node* const node = global_.TryPop()) return *node;
    return *new Node{};
  }

  static void Release(Node& node) noexcept {
    if (local_.TryPush(node)) return;
    global_.Push(node);
  }

 private:
  class Global final {
   public:
    constexpr Global() noexcept = default;

    ~Global() {
      list_.DisposeUnsafe([](Node& node) { delete &node; });
    }

    void Push(Node& node) noexcept { list_.Push(node); }

    Node* TryPop() noexcept { return list_.TryPop(); }

   private:
    IntrusiveStack<Node, MemberHook<&Node::free_or_vacant_hook>> list_{};
  };

  inline static /*constinit*/ Global global_{};

  class Local final {
   public:
    constexpr Local() noexcept = default;

    ~Local() {
      list_.clear_and_dispose([](Node* node) { global_.Push(*node); });
    }

    bool TryPush(Node& node) noexcept {
      if (list_.size() == kMaxSize) return false;
      list_.push_front(node);
      return true;
    }

    Node* TryPop() noexcept {
      if (list_.empty()) return nullptr;
      Node& node = list_.front();
      list_.pop_front();
      return &node;
    }

   private:
    static constexpr std::size_t kMaxSize = 1024;

    boost::intrusive::slist<
        Node, boost::intrusive::member_hook<Node, LocalFreeListHook,
                                            &Node::local_free_hook>>
        list_{};
  };

  inline static thread_local /*constinit*/ Local local_{};
};

template <typename T>
ResettableQueue<T>::ResettableQueue() {
  static_assert(std::atomic<TaggedPtr<Node>>::is_always_lock_free);
  static_assert(std::has_unique_object_representations_v<TaggedPtr<Node>>);

  Node& initial_dummy = FreeList::Acquire();
  initial_dummy.value.store(kRemoved, std::memory_order_relaxed);
  initial_dummy.next.store(nullptr, std::memory_order_relaxed);
  head_.store({&initial_dummy, 0}, std::memory_order_relaxed);
  tail_.store({&initial_dummy, 0}, std::memory_order_relaxed);
}

template <typename T>
ResettableQueue<T>::~ResettableQueue() {
  T value{};
  while (TryPop(value)) {
  }

  Node* const head = head_.load(std::memory_order_relaxed).GetDataPtr();
  const T head_value = head->value.load(std::memory_order_relaxed);
  if (head_value == kRemoved) {
    FreeList::Release(*head);
  } else {
    UASSERT(head_value == kVacantDummy);
  }

  vacant_list_.DisposeUnsafe([&](Node& node) { FreeList::Release(node); });
}

template <typename T>
auto ResettableQueue<T>::Push(T value) -> ItemHandle {
  UASSERT(IsValid(value));

  while (Node* const node = vacant_list_.TryPop()) {
    // 'node' may arrive here in the following states:
    // 1. kRemoved: fill with 'value' and enqueue
    // 2. kVacant: fill with 'value'
    // 3. kVacantDummy: set to kRemoved, skip
    //
    // 'node' may concurrently transition between those states in the order:
    // kVacant -> kVacantPopped -> kRemoved
    //
    // The practical frequency of those states is:
    // kRemoved > kVacant > kVacantPopped
    T expected = kRemoved;
    while (true) {
      UASSERT(!IsValid(expected));
      const T desired = (expected == kVacantDummy) ? kRemoved : value;
      if (node->value.compare_exchange_weak(expected, desired,
                                            std::memory_order_seq_cst,
                                            std::memory_order_relaxed)) {
        break;
      }
    }

    UASSERT(!IsValid(expected));
    if (expected == kRemoved) {
      DoPush(*node);
      return ItemHandle(*node, value);
    } else if (expected == kVacantDummy) {
      UASSERT(expected == kVacantDummy);
      continue;
    } else {
      // Vacant
      return ItemHandle(*node, value);
    }
  }

  // The vacant list is empty.
  // Note: this call might throw on an allocation failure.
  auto& node = FreeList::Acquire();
  node.value.store(value, std::memory_order_relaxed);
  DoPush(node);
  return ItemHandle(node, value);
}

template <typename T>
void ResettableQueue<T>::DoPush(Node& node) noexcept {
  // The unmodified procedure of Michael-Scott Push with ABA protection using
  // tagged pointers.
  node.next.store(nullptr, std::memory_order_relaxed);

  while (true) {
    TaggedPtr<Node> tail = tail_.load(std::memory_order_seq_cst);
    UASSERT(tail.GetDataPtr() != nullptr);
    auto& next_atomic = tail.GetDataPtr()->next;
    TaggedPtr<Node> next = next_atomic.load(std::memory_order_seq_cst);
    if (tail != tail_.load(std::memory_order_seq_cst)) continue;

    if (next == nullptr) {
      const TaggedPtr<Node> desired(&node, next.GetNextTag());
      if (next_atomic.compare_exchange_weak(next, desired,
                                            std::memory_order_seq_cst,
                                            std::memory_order_relaxed)) {
        const TaggedPtr<Node> tail_desired(&node, tail.GetNextTag());
        tail_.compare_exchange_strong(tail, tail_desired,
                                      std::memory_order_seq_cst,
                                      std::memory_order_relaxed);
        return;
      }
    } else {
      const TaggedPtr<Node> desired(next.GetDataPtr(), tail.GetNextTag());
      tail_.compare_exchange_weak(tail, desired, std::memory_order_seq_cst,
                                  std::memory_order_relaxed);
    }
  }
}

template <typename T>
bool ResettableQueue<T>::TryPop(T& value) noexcept {
  while (true) {
    T next_value = kRemoved;

    TaggedPtr<Node> head = head_.load(std::memory_order_seq_cst);
    TaggedPtr<Node> tail = tail_.load(std::memory_order_seq_cst);
    UASSERT(head.GetDataPtr() != nullptr);
    UASSERT(tail.GetDataPtr() != nullptr);
    TaggedPtr<Node> next =
        head.GetDataPtr()->next.load(std::memory_order_seq_cst);
    if (next.GetDataPtr() != nullptr) {
      // A modification of Michael-Scott Pop: we load next_value under the same
      // "load head - foo - load head" sequence.
      next_value = next.GetDataPtr()->value.load(std::memory_order_seq_cst);
    }
    if (head != head_.load(std::memory_order_seq_cst)) continue;

    if (head == tail) {
      if (next == nullptr) {
        return false;
      }
      const TaggedPtr<Node> desired(next.GetDataPtr(), tail.GetNextTag());
      tail_.compare_exchange_weak(tail, desired, std::memory_order_seq_cst,
                                  std::memory_order_relaxed);
    } else {
      UASSERT(next.GetDataPtr() != nullptr);
      auto& next_ref = *next.GetDataPtr();
      // A modification of Michael-Scott Pop: instead of just loading the
      // current node value, we mark it as dummy. This needs to be done to make
      // sure the node is not Removed and reused as vacant in Push, because in
      // this case the new value will be lost.
      if (!DoPopValue(next_ref, next_value)) continue;

      const TaggedPtr<Node> desired(&next_ref, head.GetNextTag());
      if (head_.compare_exchange_weak(head, desired, std::memory_order_seq_cst,
                                      std::memory_order_relaxed)) {
        DoPopNode(*head.GetDataPtr());
      }

      // We loop until we find a *valid* current node. This is equivalent to an
      // additional "pop until valid" loop on the caller side.
      if (IsValid(next_value)) {
        value = next_value;
        return true;
      }
    }
  }
}

template <typename T>
bool ResettableQueue<T>::DoPopValue(Node& node, T expected) noexcept {
  // 'node' may arrive here in the following states:
  // 1. user value: set to kRemoved, use the value
  // 2. Vacant: set to kVacantDummy
  // 3. kRemoved, kVacantDummy: skip
  if (IsValid(expected)) {
    // If the same value gets popped and pushed onto the same node,
    // we may set a node in the middle of the queue to kRemoved, producing
    // garbage (it will be cleaned up on successive TryPop calls).
    return node.value.compare_exchange_weak(expected, kRemoved,
                                            std::memory_order_seq_cst,
                                            std::memory_order_relaxed);
  } else if (expected == kRemoved || expected == kVacantDummy) {
    // May happen due to multiple parallel DoPopValue's.
    // May also indicate garbage produced in edge cases.
    return true;
  } else {
    // If the same Vacant value gets popped and pushed onto the same
    // node, we may set a node in the middle of the queue to kVacantDummy,
    // producing garbage (it will be cleaned up on successive TryPop calls).
    //
    // To mitigate, instead of a single kVacant, a tagged Vacant state is used.
    return node.value.compare_exchange_weak(expected, kVacantDummy,
                                            std::memory_order_seq_cst,
                                            std::memory_order_relaxed);
  }
}

template <typename T>
void ResettableQueue<T>::DoPopNode(Node& node) noexcept {
  // 'node' may arrive here in the following states:
  // 1. kRemoved: move to the free list
  // 2. kVacantDummy: set to kRemoved
  //
  // 'node' may concurrently transition between those states in the order:
  // kVacantDummy -> kRemoved
  //
  // The practical frequency of those states is:
  // kRemoved > kVacantDummy
  T expected = kVacantDummy;
  if (node.value.compare_exchange_strong(expected, kRemoved,
                                         std::memory_order_seq_cst,
                                         std::memory_order_relaxed)) {
    return;
  }

  UASSERT(expected == kRemoved);
  FreeList::Release(node);
}

template <typename T>
void ResettableQueue<T>::Remove(ResettableQueue::ItemHandle&& handle) noexcept {
  UASSERT(handle.node_);
  Node& node = *handle.node_;
  handle.node_ = nullptr;

  // See DoPopValue on the explanation why Vacant state should be tagged.
  const auto vacant_tag =
      node.next_vacant_tag.fetch_add(1, std::memory_order_relaxed);

  // 'node' may arrive here in the following states:
  // 1. user value: set to Vacant, push to the vacant list
  // 2. any other state: do nothing
  // 'node' may concurrently transition between those states.
  //
  // If element values can repeat, then it is possible that the node is Popped,
  // Pushed with the same value, and Remove steals an unrelated value.
  T expected = handle.value_;
  if (node.value.compare_exchange_strong(expected, MakeVacant(vacant_tag),
                                         std::memory_order_seq_cst,
                                         std::memory_order_relaxed)) {
    vacant_list_.Push(node);
  }
}

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
