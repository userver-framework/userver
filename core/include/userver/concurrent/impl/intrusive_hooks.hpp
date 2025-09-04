#pragma once

#include <atomic>
#include <cstddef>
#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

template <auto Member>
struct MemberHook final {
    template <typename T>
    static auto& GetHook(T& node) noexcept {
        return node.*Member;
    }
};

template <typename HookType>
struct BaseHook final {
    template <typename T>
    static HookType& GetHook(T& node) noexcept {
        return static_cast<HookType&>(node);
    }
};

template <typename HookExtractor1, typename HookExtractor2>
struct CombinedHook final {
    template <typename T>
    static auto& GetHook(T& node) noexcept {
        return HookExtractor2::GetHook(HookExtractor1::GetHook(node));
    }
};

template <typename T>
struct SinglyLinkedHook final {
    std::atomic<T*> next{nullptr};
};

template <typename T>
struct IntrusiveWalkablePoolHook final {
    SinglyLinkedHook<T> permanent_list_hook;
    SinglyLinkedHook<T> free_list_hook;
};

// IntrusiveMpscQueue element types must inherit from this.
class SinglyLinkedBaseHook {
public:
    SinglyLinkedHook<SinglyLinkedBaseHook> singly_linked_hook;
};

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
