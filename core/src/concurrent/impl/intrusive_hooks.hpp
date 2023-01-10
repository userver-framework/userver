#pragma once

#include <atomic>
#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

template <auto Member>
struct MemberHook final {
  template <typename T>
  auto& operator()(T& node) const noexcept {
    return node.*Member;
  }
};

template <typename HookExtractor1, typename HookExtractor2>
struct CombinedHook final {
  static_assert(std::is_empty_v<HookExtractor1>);
  static_assert(std::is_empty_v<HookExtractor2>);

  template <typename T>
  auto& operator()(T& node) const noexcept {
    return HookExtractor2{}(HookExtractor1{}(node));
  }
};

template <typename T>
class SinglyLinkedHook final {
 public:
  SinglyLinkedHook() = default;

 private:
  template <typename U, typename HookExtractor>
  friend class IntrusiveStack;

  std::atomic<T*> next_{nullptr};
};

template <typename T>
struct IntrusiveWalkablePoolHook final {
  SinglyLinkedHook<T> permanent_list_hook;
  SinglyLinkedHook<T> free_list_hook;
};

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
