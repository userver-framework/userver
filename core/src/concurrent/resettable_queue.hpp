#pragma once

#include <atomic>
#include <type_traits>

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
/// 1. be trivially-copyable
/// 2. have "invalid" states, which are never Pushed normally, and provide
///    access to them using...
/// 2.1. `T::MakeInvalid(std::size_t invalid_id) -> T`
/// 3. be equality-comparable using `operator==`
///
/// The inserted elements must be "reasonably unique" for the queue to work
/// correctly (see the details below).
template <typename T>
class ResettableQueue final {
 public:
  static constexpr std::size_t kInvalidStateCount = 4;

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
  enum class StorageMode {
    kStatic,
    kDynamic,
  };

  static constexpr T MakeInvalid(std::size_t invalid_id) noexcept {
    UASSERT(invalid_id < kInvalidStateCount);
    return T::MakeInvalid(invalid_id);
  }

  // The node has been physically removed from the queue and is free.
  inline static const T kPhysicallyRemoved = MakeInvalid(0);

  // The value of the node has been extracted in TryPop. The node is thus
  // logically removed. However, it is still needed by the queue and is not
  // free.
  inline static const T kPopped = MakeInvalid(1);

  // The value of the node has been cleared in Remove. The node has still not
  // been popped. So the node may be reused without a proper push-pop sequence.
  inline static const T kVacant = MakeInvalid(2);

  // A kVacant node that was then kPopped. The consumer can't remove the node
  // from the free list, so it will be cleaned up by the next Push.
  inline static const T kVacantPopped = MakeInvalid(3);

  struct Node final {
    Node() noexcept : Node(StorageMode::kStatic) {}
    explicit Node(StorageMode storage_mode) noexcept
        : storage_mode(storage_mode) {}

    FastAtomic<T> value{kPhysicallyRemoved};
    std::atomic<TaggedPtr<Node>> next{nullptr};
    IntrusiveStackHook<Node> free_list_hook{};
    const StorageMode storage_mode;
  };

  void DoPush(Node& node) noexcept;
  bool DoPopValue(Node& node, T expected) noexcept;
  void DoPopNode(Node& node) noexcept;

  static constexpr std::size_t kStaticNodesCount = 2;

  Node static_nodes_[kStaticNodesCount]{};
  std::atomic<TaggedPtr<Node>> head_{{&static_nodes_[0], 0}};
  std::atomic<TaggedPtr<Node>> tail_{{&static_nodes_[0], 0}};
  IntrusiveStack<Node, MemberHook<&Node::free_list_hook>> free_list_{};
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

// TODO take note that 'load' and 'store' on FastAtomic are actually CAS.
// TODO relax load-store if possible.
template <typename T>
ResettableQueue<T>::ResettableQueue() {
  static_assert(std::atomic<TaggedPtr<Node>>::is_always_lock_free);
  static_assert(std::has_unique_object_representations_v<TaggedPtr<Node>>);
  static_assert(std::is_trivially_copyable_v<T>);
  static_assert(std::has_unique_object_representations_v<T>);

  static_nodes_[0].value.store(kPopped, std::memory_order_relaxed);
  for (std::size_t i = 1; i != kStaticNodesCount; ++i) {
    free_list_.Push(static_nodes_[i]);
  }
}

template <typename T>
ResettableQueue<T>::~ResettableQueue() {
  {
    T value{};
    while (TryPop(value)) {
    }
  }
  const auto disposer = [](Node& node) {
    if (node.storage_mode == StorageMode::kDynamic) {
      delete &node;
    }
  };
  free_list_.DisposeUnsafe(disposer);
  {
    Node* const head = head_.load(std::memory_order_relaxed).GetDataPtr();
    UASSERT(head);
    disposer(*head);
  }
}

template <typename T>
auto ResettableQueue<T>::Push(T value) -> ItemHandle {
  UASSERT(!(value == kPhysicallyRemoved));
  UASSERT(!(value == kPopped));
  UASSERT(!(value == kVacant));
  UASSERT(!(value == kVacantPopped));

  while (Node* const node = free_list_.TryPop()) {
    // 'node' may arrive here in the following states:
    // 1. kPhysicallyRemoved: fill with 'value' and enqueue
    // 2. kVacant: fill with 'value'
    // 3. kVacantPopped: set to kPopped, skip
    //
    // 'node' may concurrently transition between those states in the order:
    // kVacant -> kVacantPopped -> kPhysicallyRemoved
    //
    // The practical frequency of those states is:
    // kPhysicallyRemoved > kVacant > kVacantPopped
    T expected = kPhysicallyRemoved;
    if (node->value.compare_exchange_strong(expected, value,
                                            std::memory_order_release,
                                            std::memory_order_relaxed)) {
      DoPush(*node);
      return ItemHandle(*node, value);
    }

    while (true) {
      const T desired = (expected == kVacantPopped) ? kPopped : value;
      if (node->value.compare_exchange_strong(expected, desired,
                                              std::memory_order_release,
                                              std::memory_order_relaxed)) {
        break;
      }
    }

    if (expected == kPhysicallyRemoved) {
      DoPush(*node);
      return ItemHandle(*node, value);
    } else if (expected == kVacant) {
      return ItemHandle(*node, value);
    } else {
      UASSERT(expected == kVacantPopped);
      continue;
    }
  }

  // The free list is empty.
  auto& node = *new Node();
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
    T next_value = kPhysicallyRemoved;

    TaggedPtr<Node> head = head_.load(std::memory_order_seq_cst);
    TaggedPtr<Node> tail = tail_.load(std::memory_order_seq_cst);
    UASSERT(head.GetDataPtr() != nullptr);
    UASSERT(tail.GetDataPtr() != nullptr);
    TaggedPtr<Node> next =
        head.GetDataPtr()->next.load(std::memory_order_seq_cst);
    if (next.GetDataPtr() != nullptr) {
      // A modification of Michael-Scott Pop: we load next_value under the same
      // load head - foo - load head "lock".
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
      // current node value, we mark it as popped. This needs to be done to make
      // sure the node is not Removed and reused, because in this case the
      // replaced value will be lost.
      if (!DoPopValue(next_ref, next_value)) continue;

      const TaggedPtr<Node> desired(&next_ref, head.GetNextTag());
      if (head_.compare_exchange_weak(head, desired, std::memory_order_seq_cst,
                                      std::memory_order_relaxed)) {
        DoPopNode(*head.GetDataPtr());
      }

      // We loop until we find a *valid* current node.
      if (!(next_value == kVacant) && !(next_value == kPopped) &&
          !(next_value == kVacantPopped)) {
        value = next_value;
        return true;
      }
    }
  }
}

template <typename T>
bool ResettableQueue<T>::DoPopValue(Node& node, T expected) noexcept {
  // 'node' may arrive here in the following states:
  // 1. user value: set to kPopped, use the value
  // 2. kVacant: set to kVacantPopped
  // 3. kPopped, kVacantPopped: do nothing
  //
  // 'node' may concurrently transition between those states in the order:
  //
  // The practical frequency of those states is:
  // user value > kVacant > kVacantPopped > kPopped
  UASSERT(!(expected == kPhysicallyRemoved));
  UASSERT_MSG(!(expected == kPopped),
              "The same value has been pushed in a short amount of time.");

  if (expected == kPopped || expected == kVacantPopped) {
    // Edge cases, described below.
    return true;
  } else if (expected == kVacant) {
    // Annoyance: if the same kVacant value gets popped and pushed onto the same
    // node, we may set a node in the middle of the queue to kVacantPopped,
    // producing garbage (it will be cleaned up on successive TryPop calls).
    //
    // TODO mitigate by tagging kVacant values
    return node.value.compare_exchange_weak(expected, kVacantPopped,
                                            std::memory_order_seq_cst,
                                            std::memory_order_relaxed);
  } else {
    // If the same value gets popped and pushed onto the same node,
    // we may set a node in the middle of the queue to kPopped, producing
    // garbage (it will be cleaned up on successive TryPop calls).
    return node.value.compare_exchange_weak(expected, kPopped,
                                            std::memory_order_seq_cst,
                                            std::memory_order_relaxed);
  }
}

template <typename T>
void ResettableQueue<T>::DoPopNode(Node& node) noexcept {
  // 'node' may arrive here in the following states:
  // 1. kPopped: set to kPhysicallyRemoved, move to the free list
  // 2. kVacantPopped: set to kPhysicallyRemoved, already in the free list
  //
  // 'node' may concurrently transition between those states in the order:
  // kVacantPopped -> kPopped
  //
  // The practical frequency of those states is:
  // kPopped > kVacantPopped
  T expected = kVacantPopped;
  if (node.value.compare_exchange_strong(expected, kPhysicallyRemoved,
                                         std::memory_order_seq_cst,
                                         std::memory_order_relaxed)) {
    return;
  }

  UASSERT(expected == kPopped);
  expected = kPopped;
  const bool success = node.value.compare_exchange_strong(
      expected, kPhysicallyRemoved, std::memory_order_seq_cst,
      std::memory_order_relaxed);
  UASSERT(success);
  free_list_.Push(node);
}

template <typename T>
void ResettableQueue<T>::Remove(ResettableQueue::ItemHandle&& handle) noexcept {
  UASSERT(handle.node_);
  Node& node = *handle.node_;
  handle.node_ = nullptr;

  // 'node' may arrive here in the following states:
  // 1. user value: set to kVacant, add to the free list
  // 2. any invalid state: do nothing
  // 'node' may concurrently transition between those states.
  //
  // If element values can repeat, then it is possible that the node is Popped,
  // Pushed with the same value, and Remove steals an unrelated value.
  T expected = handle.value_;
  if (node.value.compare_exchange_strong(expected, kVacant,
                                         std::memory_order_seq_cst,
                                         std::memory_order_relaxed)) {
    free_list_.Push(node);
  }
}

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
