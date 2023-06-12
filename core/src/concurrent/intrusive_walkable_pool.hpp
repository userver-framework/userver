#pragma once

#include <atomic>
#include <cstdint>
#include <type_traits>

#include <concurrent/impl/intrusive_hooks.hpp>
#include <concurrent/impl/tagged_ptr.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

/// @brief An intrusive stack of nodes of type `T` with ABA protection.
///
/// - The IntrusiveStack does not own the nodes. The user is responsible
///   for deleting them, e.g. by calling `Pop` repeatedly
/// - The element type `T` must contain IntrusiveStackHook,
///   extracted by `HookExtractor`
/// - The objects are not destroyed on insertion into IntrusiveStack
/// - If a node is `Pop`-ed from an IntrusiveStack, it must not be destroyed
///   until the IntrusiveStack stops being used
///
/// Implemented using Treiber stack with counted pointers. See
/// Treiber, R.K., 1986. Systems programming: Coping with parallelism.
template <typename T, typename HookExtractor>
class IntrusiveStack final {
  static_assert(std::is_empty_v<HookExtractor>);
  static_assert(std::is_invocable_r_v<SinglyLinkedHook<T>&, HookExtractor, T&>);

 public:
  IntrusiveStack() = default;

  IntrusiveStack(IntrusiveStack&&) = delete;
  IntrusiveStack& operator=(IntrusiveStack&&) = delete;

  void Push(T& node) noexcept {
    UASSERT_MSG(GetNext(node).load(std::memory_order_relaxed) == nullptr,
                "This node is already contained in an IntrusiveStack");

    NodeTaggedPtr expected = stack_head_.load();
    while (true) {
      GetNext(node).store(expected.GetDataPtr());
      const NodeTaggedPtr desired(&node, expected.GetTag());
      if (stack_head_.compare_exchange_weak(expected, desired)) {
        break;
      }
    }
  }

  T* TryPop() noexcept {
    NodeTaggedPtr expected = stack_head_.load();
    while (true) {
      T* const expected_ptr = expected.GetDataPtr();
      if (!expected_ptr) return nullptr;
      const NodeTaggedPtr desired(GetNext(*expected_ptr).load(),
                                  expected.GetNextTag());
      if (stack_head_.compare_exchange_weak(expected, desired)) {
        // 'relaxed' is OK, because popping a node must happen-before pushing it
        GetNext(*expected_ptr).store(nullptr, std::memory_order_relaxed);
        return expected_ptr;
      }
    }
  }

  template <typename Func>
  void WalkUnsafe(const Func& func) {
    DoWalk<T&>(func);
  }

  template <typename Func>
  void WalkUnsafe(const Func& func) const {
    DoWalk<const T&>(func);
  }

  template <typename DisposerFunc>
  void DisposeUnsafe(const DisposerFunc& disposer) noexcept {
    T* iter = stack_head_.load().GetDataPtr();
    stack_head_.store(nullptr);
    while (iter) {
      T* const old_iter = iter;
      iter = GetNext(*iter).load();
      disposer(*old_iter);
    }
  }

  std::size_t GetSizeUnsafe() const noexcept {
    std::size_t size = 0;
    WalkUnsafe([&](auto& /*item*/) { ++size; });
    return size;
  }

 private:
  using NodeTaggedPtr = TaggedPtr<T>;

  static_assert(std::atomic<NodeTaggedPtr>::is_always_lock_free);
  static_assert(std::has_unique_object_representations_v<NodeTaggedPtr>);

  static std::atomic<T*>& GetNext(T& node) noexcept {
    return static_cast<SinglyLinkedHook<T>&>(HookExtractor{}(node)).next_;
  }

  template <typename U, typename Func>
  void DoWalk(const Func& func) const {
    for (auto* iter = stack_head_.load().GetDataPtr(); iter;
         iter = GetNext(*iter).load()) {
      func(static_cast<U>(*iter));
    }
  }

  std::atomic<NodeTaggedPtr> stack_head_{nullptr};
};

/// @brief A walkable pool of nodes of type `T`
///
/// - The IntrusiveWalkablePool does own the nodes. They are created and deleted
///   by standard `new` and `delete`
/// - The element type `T` must contain IntrusiveWalkablePoolHook,
///   extracted by `HookExtractor`
/// - The nodes are only destroyed on the pool destruction, so the node count is
///   monotonically non-decreasing
template <typename T, typename HookExtractor>
class IntrusiveWalkablePool final {
  static_assert(std::is_empty_v<HookExtractor>);
  static_assert(
      std::is_invocable_r_v<IntrusiveWalkablePoolHook<T>&, HookExtractor, T&>);

 public:
  IntrusiveWalkablePool() = default;

  IntrusiveWalkablePool(IntrusiveWalkablePool&&) = delete;
  IntrusiveWalkablePool& operator=(IntrusiveWalkablePool&&) = delete;

  ~IntrusiveWalkablePool() {
    UASSERT_MSG(free_list_.GetSizeUnsafe() == permanent_list_.GetSizeUnsafe(),
                "Some acquired nodes are still being held at IWP destruction");
    permanent_list_.DisposeUnsafe([](T& node) { delete &node; });
  }

  /// Takes a free node, if any, or default-constructs one
  T& Acquire() {
    return Acquire([] { return T(); });
  }

  /// - If there is a free node in the IntrusiveWalkablePool, returns it. It
  ///   keeps the same value as the one before its previous `Release`
  /// - If there are no free nodes, creates a new one using `factory`
  template <typename Factory>
  T& Acquire(const Factory& factory) {
    if (auto* node = free_list_.TryPop()) {
      return *node;
    }
    auto* new_node = new T(factory());
    permanent_list_.Push(*new_node);
    return *new_node;
  }

  /// Makes the node available for reuse by `Acquire`. Does not touch the node
  /// data.
  void Release(T& node) noexcept { free_list_.Push(node); }

  /// Walks over all the nodes, both free and acquired
  template <typename Func>
  void Walk(const Func& func) {
    permanent_list_.WalkUnsafe(func);
  }

  /// Walks over all the nodes, both free and acquired
  template <typename Func>
  void Walk(const Func& func) const {
    permanent_list_.WalkUnsafe(func);
  }

 private:
  using PermanentListHookExtractor = CombinedHook<
      HookExtractor,
      MemberHook<&IntrusiveWalkablePoolHook<T>::permanent_list_hook>>;
  using FreeListHookExtractor =
      CombinedHook<HookExtractor,
                   MemberHook<&IntrusiveWalkablePoolHook<T>::free_list_hook>>;

  IntrusiveStack<T, PermanentListHookExtractor> permanent_list_;
  IntrusiveStack<T, FreeListHookExtractor> free_list_;
};

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
