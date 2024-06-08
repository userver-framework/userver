#pragma once

#include <cstdint>
#include <type_traits>

#include <userver/concurrent/impl/intrusive_hooks.hpp>
#include <userver/concurrent/impl/intrusive_stack.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

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
