#pragma once

#include <atomic>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

template <auto Member>
struct MemberHook final {
  template <typename T>
  auto& operator()(T& node) const noexcept {
    return node.*Member;
  }
};

template <typename T>
class SinglyLinkedHook final {
 public:
  SinglyLinkedHook() = default;

 private:
  template <typename U, typename HookExtractor>
  friend class IntrusiveStack;

  template <typename U, typename HookExtractor>
  friend class IntrusiveMpscQueue;

  std::atomic<T*> next_{nullptr};
};

template <typename T>
struct IntrusiveWalkablePoolHook final {
  SinglyLinkedHook<T> permanent_list_hook;
  SinglyLinkedHook<T> free_list_hook;
};

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
