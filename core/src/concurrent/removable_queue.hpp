#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <utility>

#include <boost/intrusive/slist.hpp>
#include <boost/intrusive/slist_hook.hpp>

#include <concurrent/intrusive_walkable_pool.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

// A state-preserving allocator for nodes that must never be freed.
template <typename NodeType, std::size_t LocalCacheSize>
class FreeList final {
 public:
  static NodeType& Acquire() {
    if (NodeType* const node = local_.TryPop()) return *node;
    if (NodeType* const node = global_.TryPop()) return *node;
    return *new NodeType{};
  }

  static void Release(NodeType& node) noexcept {
    if (local_.TryPush(node)) return;
    global_.Push(node);
  }

 private:
  class Global final {
   public:
    constexpr Global() noexcept = default;

    ~Global() {
      list_.DisposeUnsafe([](NodeType& node) { delete &node; });
    }

    void Push(NodeType& node) noexcept { list_.Push(node); }

    NodeType* TryPop() noexcept { return list_.TryPop(); }

   private:
    IntrusiveStack<NodeType, MemberHook<&NodeType::global_free_list_hook>>
        list_;
  };

  inline static /*constinit*/ Global global_{};

  class Local final {
   public:
    constexpr Local() noexcept = default;

    ~Local() {
      list_.clear_and_dispose([](NodeType* node) { global_.Push(*node); });
    }

    bool TryPush(NodeType& node) noexcept {
      if (list_.size() == LocalCacheSize) return false;
      list_.push_front(node);
      return true;
    }

    NodeType* TryPop() noexcept {
      if (list_.empty()) return nullptr;
      NodeType& node = list_.front();
      list_.pop_front();
      return &node;
    }

   private:
    using LocalListHook =
        decltype(std::declval<NodeType&>().local_free_list_hook);

    using LocalList = boost::intrusive::slist<
        NodeType,
        boost::intrusive::member_hook<NodeType, LocalListHook,
                                      &NodeType::local_free_list_hook>>;

    LocalList list_{};
  };

  inline static thread_local /*constinit*/ Local local_{};
};

/// @brief A queue that allows to remove elements from the middle.
///
/// Elements in a typical lock-free queue may be removed by CAS-ing them away.
/// However, this leaves garbage nodes in the middle of the queue. If pops are
/// infrequent, then elements are only removed from the middle, and this may
/// become a memory leak. This modified queue attempts to resolve those issues
/// by reusing the "vacant" nodes.
///
/// The queue provides k-FIFO ordering for Push and TryPop: no more than
/// kQuasiFactor elements may go out-of-order before each element.
template <typename T>
class RemovableQueue final {
 public:
  RemovableQueue();

  RemovableQueue(RemovableQueue&&) = delete;
  RemovableQueue& operator=(RemovableQueue&&) = delete;
  ~RemovableQueue();

  class ItemHandle;

  /// Enqueue an element. Returns an `ItemHandle`, which can be used to `Remove`
  /// the element later.
  ItemHandle Push(T&& value);

  /// Dequeue an element, if the queue is not empty.
  bool TryPop(T& value) noexcept;

  /// Move the originally Pushed element into `value`, if not TryPopped yet.
  bool Remove(ItemHandle&& handle, T& value) noexcept;

 private:
  // The implementation of the queue is based on Segmented Queue from:
  //
  // Y. Afek, G. Korland, and E. Yanovsky. Quasi-linearizability: Relaxed
  // consistency for improved concurrency. In Proc. Conference on Principles
  // of Distributed Systems (OPODIS), pages 395–410. Springer, 2010.

  // The paper suggests a good quasi factor value of 64, but we optimize
  // for the non-contended case and a lower memory usage.
  static constexpr std::size_t kQuasiFactor = 8;

  template <typename NodeType>
  using LocalListHook = boost::intrusive::slist_member_hook<
      boost::intrusive::link_mode<boost::intrusive::normal_link>,
      boost::intrusive::void_pointer<NodeType*>>;

  struct Node;
  struct Slot;

  // All node pointers are protected with the segment epoch.
  // Possible slot states: null, filled, vacant, deleted.

  // Empty slot at the tail of the queue.
  static constexpr std::uint64_t kNullNodeRaw = 1 << 0;
  inline static const auto kNullNode = reinterpret_cast<Node*>(kNullNodeRaw);

  // Empty slot in the head of the queue.
  static constexpr std::uint64_t kDeletedNodeRaw = 1 << 1;
  inline static const auto kDeletedNode =
      reinterpret_cast<Node*>(kDeletedNodeRaw);

  // Empty slot in the middle of the queue.
  inline static const auto kVacantNode = reinterpret_cast<Node*>(1 << 2);

  // Protects Segments and Slots against ABA.
  using SegmentEpoch = std::uint16_t;
  // Additionally protects Nodes against removing a newer element.
  using NodeEpoch = std::uint64_t;

  // A node is owned by the respective Slot (if active), by the vacant list (if
  // vacant), or by the free list. Nodes must outlive the queues they have been
  // used in.
  struct Node final {
    std::optional<T> value;
    TaggedPtr<Slot> vacant_slot{nullptr};
    std::atomic<NodeEpoch> node_epoch{0};
    IntrusiveStackHook<Node> global_free_list_hook;
    LocalListHook<Node> local_free_list_hook;
  };

  // Should be aligned to reduce contention, but we trade it for single-threaded
  // performance and a lower memory usage.
  struct Slot final {
    std::atomic<TaggedPtr<Node>> node{{kNullNode, 0}};
  };

  // Segments are the nodes of the overall batched queue. Segments must outlive
  // the queues they have been used in.
  struct alignas(64) Segment final {
    Slot slots[kQuasiFactor]{};
    std::atomic<TaggedPtr<Segment>> next{nullptr};
    SegmentEpoch epoch{0};
    IntrusiveStackHook<Segment> global_free_list_hook{};
    LocalListHook<Segment> local_free_list_hook{};
  };

  using NodeFreeList = FreeList<Node, 1024>;
  using SegmentFreeList = FreeList<Segment, 512>;

  void DisposeSegment(Segment& segment) noexcept;

  // Segments form a lock-free singly-linked list. These operations form a basis
  // for the Segmented Queue implementation. Unlike in the classic "counted
  // pointers" scheme, we tag pointers to Segments with their own epoch, unless
  // it's nullptr, in which case it's the epoch of the next pointer's owner.
  // The implementation is inspired by Michael-Scott queue.
  TaggedPtr<Segment> GetLast() noexcept;
  TaggedPtr<Segment> GetFirst() noexcept;
  TaggedPtr<Segment> CreateLast(TaggedPtr<Segment> expected_tail);
  TaggedPtr<Segment> RemoveFirst(TaggedPtr<Segment> expected_head) noexcept;

  // Main operations from the Segmented Queue paper.
  TaggedPtr<Slot> DoPush(Node& node);
  Node* DoTryPop() noexcept;
  std::optional<Node*> DoTryPopStep(TaggedPtr<Segment> segment) noexcept;

  std::atomic<TaggedPtr<Segment>> head_{nullptr};
  std::atomic<TaggedPtr<Segment>> tail_{nullptr};
  IntrusiveStack<Node, MemberHook<&Node::global_free_list_hook>> vacant_list_{};
};

template <typename T>
class RemovableQueue<T>::ItemHandle final {
 public:
  ItemHandle() noexcept = default;

 private:
  friend class RemovableQueue;

  ItemHandle(Slot& slot, TaggedPtr<Node> node, NodeEpoch node_epoch) noexcept
      : slot_(&slot), node_(node), node_epoch_(node_epoch) {}

  Slot* slot_{nullptr};
  TaggedPtr<Node> node_{nullptr};
  NodeEpoch node_epoch_{0};
};

template <typename T>
RemovableQueue<T>::RemovableQueue() {
  Segment& segment = SegmentFreeList::Acquire();
  UASSERT(segment.next.load(std::memory_order_relaxed) ==
          TaggedPtr<Segment>(nullptr, segment.epoch));
  TaggedPtr tagged_segment(&segment, segment.epoch);
  head_.store(tagged_segment, std::memory_order_relaxed);
  tail_.store(tagged_segment, std::memory_order_relaxed);
}

template <typename T>
RemovableQueue<T>::~RemovableQueue() {
  T value{};
  while (TryPop(value)) {
  }

  Segment* const head = head_.load(std::memory_order_relaxed).GetDataPtr();
  UASSERT(head != nullptr);
  DisposeSegment(*head);

  vacant_list_.DisposeUnsafe([&](Node& node) {
    UASSERT(!node.value);
    node.vacant_slot = nullptr;
    NodeFreeList::Release(node);
  });
}

template <typename T>
auto RemovableQueue<T>::Push(T&& value) -> ItemHandle {
  static_assert(std::is_nothrow_move_constructible_v<T>);

  Node* node = vacant_list_.TryPop();

  if (node != nullptr) {
    UASSERT(!node->value);
    const auto slot_tagged = node->vacant_slot;
    UASSERT(slot_tagged != nullptr);
    auto& slot = *slot_tagged.GetDataPtr();
    const auto segment_epoch = slot_tagged.GetTag();
    const auto node_epoch = node->node_epoch.load(std::memory_order_relaxed);
    node->value.emplace(std::move(value));
    node->vacant_slot = nullptr;

    TaggedPtr expected(kVacantNode, segment_epoch);
    const TaggedPtr desired(node, segment_epoch);
    const bool success = slot.node.compare_exchange_strong(
        expected, desired, std::memory_order_seq_cst,
        std::memory_order_relaxed);

    if (success) {
      return ItemHandle(slot, desired, node_epoch);
    }
  } else {
    node = &NodeFreeList::Acquire();
    UASSERT(!node->value);
    node->value.emplace(std::move(value));
  }

  UASSERT(node->vacant_slot == nullptr);
  const auto node_epoch = node->node_epoch.load(std::memory_order_relaxed);
  const auto slot_tagged = DoPush(*node);
  return ItemHandle(*slot_tagged.GetDataPtr(),
                    TaggedPtr(node, slot_tagged.GetTag()), node_epoch);
}

template <typename T>
bool RemovableQueue<T>::TryPop(T& value) noexcept {
  static_assert(std::is_nothrow_move_assignable_v<T>);

  auto* const node = DoTryPop();
  if (!node) return false;

  UASSERT(node->value);
  UASSERT(node->vacant_slot == nullptr);
  value = std::move(*node->value);
  node->value.reset();
  node->node_epoch.store(node->node_epoch.load(std::memory_order_relaxed) + 1,
                         std::memory_order_relaxed);
  NodeFreeList::Release(*node);
  return true;
}

template <typename T>
bool RemovableQueue<T>::Remove(RemovableQueue::ItemHandle&& handle,
                               T& value) noexcept {
  static_assert(std::is_nothrow_move_assignable_v<T>);

  UASSERT(handle.slot_);
  Slot& slot = *handle.slot_;
  handle.slot_ = nullptr;

  auto node_tagged = handle.node_;
  UASSERT(node_tagged.GetDataPtr() != nullptr);
  Node& node = *node_tagged.GetDataPtr();

  const auto node_epoch = handle.node_epoch_;
  if (node.node_epoch.load(std::memory_order_relaxed) != node_epoch) {
    return false;
  }

  const auto segment_epoch = node_tagged.GetTag();
  TaggedPtr desired(kVacantNode, segment_epoch);

  if (slot.node.load(std::memory_order_seq_cst) == node_tagged &&
      slot.node.compare_exchange_strong(node_tagged, desired,
                                        std::memory_order_seq_cst,
                                        std::memory_order_relaxed)) {
    UASSERT(node.value);
    value = *node.value;
    node.value.reset();
    node.vacant_slot = TaggedPtr(&slot, segment_epoch);
    vacant_list_.Push(node);
    return true;
  } else {
    return false;
  }
}

template <typename T>
void RemovableQueue<T>::DisposeSegment(Segment& segment) noexcept {
  const SegmentEpoch new_segment_epoch = segment.epoch + 1;
  segment.epoch = new_segment_epoch;

  TaggedPtr<Node> null_node(kNullNode, new_segment_epoch);
  for (Slot& slot : segment.slots) {
    slot.node.store(null_node, std::memory_order_relaxed);
  }

  segment.next.store({nullptr, new_segment_epoch}, std::memory_order_relaxed);
  SegmentFreeList::Release(segment);
}

template <typename T>
auto RemovableQueue<T>::GetLast() noexcept -> TaggedPtr<Segment> {
  while (true) {
    auto tail_tagged = tail_.load(std::memory_order_seq_cst);
    auto* const tail = tail_tagged.GetDataPtr();
    UASSERT(tail != nullptr);

    const auto next = tail->next.load(std::memory_order_seq_cst);
    if (next.GetDataPtr() == nullptr) {
      return tail_tagged;
    }

    tail_.compare_exchange_weak(tail_tagged, next, std::memory_order_relaxed,
                                std::memory_order_relaxed);
  }
}

template <typename T>
auto RemovableQueue<T>::GetFirst() noexcept -> TaggedPtr<Segment> {
  return head_.load(std::memory_order_acquire);
}

template <typename T>
auto RemovableQueue<T>::CreateLast(TaggedPtr<Segment> expected_tail)
    -> TaggedPtr<Segment> {
  UASSERT(expected_tail.GetDataPtr() != nullptr);

  auto tail_tagged = tail_.load(std::memory_order_acquire);
  auto* const tail = tail_tagged.GetDataPtr();
  UASSERT(tail != nullptr);
  TaggedPtr<Segment> expected_next(nullptr, tail_tagged.GetTag());

  if (tail_tagged != expected_tail ||
      tail->next.load(std::memory_order_acquire) != expected_next) {
    // Tail moved.
    return GetLast();
  }

  Segment& segment = SegmentFreeList::Acquire();
  UASSERT(segment.next.load(std::memory_order_relaxed) ==
          TaggedPtr<Segment>(nullptr, segment.epoch));
  TaggedPtr segment_tagged(&segment, segment.epoch);

  if (!tail->next.compare_exchange_strong(expected_next, segment_tagged,
                                          std::memory_order_release,
                                          std::memory_order_relaxed)) {
    // Tail moved.
    SegmentFreeList::Release(segment);
    return GetLast();
  }

  // Success, now try to help by fixing tail. If we fail, it means someone has
  // already helped us.
  tail_.compare_exchange_strong(tail_tagged, segment_tagged,
                                std::memory_order_release,
                                std::memory_order_relaxed);
  return segment_tagged;
}

template <typename T>
auto RemovableQueue<T>::RemoveFirst(TaggedPtr<Segment> expected_head) noexcept
    -> TaggedPtr<Segment> {
  UASSERT(expected_head.GetDataPtr() != nullptr);

  if (head_.load(std::memory_order_seq_cst) != expected_head) {
    return GetFirst();
  }

  auto* const head = expected_head.GetDataPtr();
  const auto next_tagged = head->next.load(std::memory_order_seq_cst);
  if (next_tagged.GetDataPtr() == nullptr) {
    // Signal that the queue is empty.
    return nullptr;
  }

  // tail_ either points to:
  // 1. expected_head -> need to help once to correct to at least 'next_tagged'
  // 2. expected_head + N -> no need to help
  // Trying to help just once is enough.
  auto expected_tail = expected_head;
  if (tail_.load(std::memory_order_seq_cst) == expected_head) {
    tail_.compare_exchange_strong(expected_tail, next_tagged,
                                  std::memory_order_seq_cst,
                                  std::memory_order_relaxed);
  }

  if (!head_.compare_exchange_strong(expected_head, next_tagged,
                                     std::memory_order_seq_cst,
                                     std::memory_order_relaxed)) {
    return GetFirst();
  }

  DisposeSegment(*head);
  return next_tagged;
}

template <typename T>
auto RemovableQueue<T>::DoPush(Node& node) -> TaggedPtr<Slot> {
  UASSERT(node.value);
  auto last_segment_tagged = GetLast();

  while (true) {
    auto* const last_segment = last_segment_tagged.GetDataPtr();
    UASSERT(last_segment != nullptr);

    const auto segment_epoch = last_segment_tagged.GetTag();
    const TaggedPtr<Node> node_tagged(&node, segment_epoch);
    const TaggedPtr<Node> null_node(kNullNode, segment_epoch);

    // The paper advises randomized iteration order for better performance under
    // contention, but we trade it for better single-threaded performance.
    for (auto& slot : last_segment->slots) {
      auto expected = null_node;
      if (slot.node.load(std::memory_order_seq_cst) == expected &&
          slot.node.compare_exchange_strong(expected, node_tagged,
                                            std::memory_order_seq_cst,
                                            std::memory_order_relaxed)) {
        return TaggedPtr(&slot, segment_epoch);
      }
    }

    last_segment_tagged = CreateLast(last_segment_tagged);
  }
}

template <typename T>
auto RemovableQueue<T>::DoTryPop() noexcept -> Node* {
  auto first_segment_tagged = GetFirst();

  while (true) {
    const auto result = DoTryPopStep(first_segment_tagged);
    if (result) return *result;
    first_segment_tagged = RemoveFirst(first_segment_tagged);
  }
}

template <typename T>
auto RemovableQueue<T>::DoTryPopStep(TaggedPtr<Segment> segment) noexcept
    -> std::optional<Node*> {
  auto* const first_segment = segment.GetDataPtr();
  if (first_segment == nullptr) return nullptr;

  const auto segment_epoch = segment.GetTag();
  const TaggedPtr<Node> deleted_node(kDeletedNode, segment_epoch);

  std::uint64_t node_flags = 0;

  // The paper advises randomized iteration order for better performance under
  // contention, but we trade it for better single-threaded performance.
  for (auto& slot : first_segment->slots) {
    auto node_tagged = slot.node.load(std::memory_order_seq_cst);

    while (true) {
      if ((node_tagged.GetRaw() & (kNullNodeRaw | kDeletedNodeRaw)) != 0) {
        node_flags |= node_tagged.GetRaw();
        break;  // keep looking for a filled slot
      }

      if (node_tagged.GetTag() != segment_epoch) return std::nullopt;

      if (slot.node.compare_exchange_weak(node_tagged, deleted_node,
                                          std::memory_order_seq_cst,
                                          std::memory_order_relaxed)) {
        auto* const node = node_tagged.GetDataPtr();
        if (node == kVacantNode) {
          // The node is still owned by the vacant list.
          // The vacant re-insertion will fail.
          break;
        } else {
          return node;
        }
      }
    }
  }

  // Optimized hadNullValue check.
  if ((node_flags & kNullNodeRaw) != 0) return nullptr;

  return std::nullopt;
}

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
