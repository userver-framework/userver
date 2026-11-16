#pragma once

#include <userver/concurrent/impl/intrusive_hooks.hpp>
#include <userver/concurrent/impl/intrusive_stack.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {

template <typename T>
struct TimerStorageHook final {
    concurrent::impl::SinglyLinkedHook<T> all_hook;
    concurrent::impl::SinglyLinkedHook<T> free_hook;
};

/// Nodes for timers that are acquired on one thread and released on another.
/// Released nodes can be reused by Acquire on any thread. Concurrent acquisitions
/// may allocate additional nodes while other acquisitions or releases are in flight.
/// Nodes keep their addresses and are destroyed only with the storage.
template <typename T, typename HookExtractor>
class TimerStorage final {
public:
    TimerStorage() = default;
    TimerStorage(TimerStorage&&) = delete;
    TimerStorage& operator=(TimerStorage&&) = delete;

    /// Requires that every node is released and no thread uses the storage.
    ~TimerStorage() {
        UASSERT_MSG(
            free_.GetSizeUnsafe() == all_.GetSizeUnsafe(),
            "Release every timer before destroying TimerStorage"
        );
        all_.DisposeUnsafe([](T& node) { delete &node; });
    }

    /// Concurrent. Calls `factory` if no released node is found.
    template <typename Factory>
    T& Acquire(const Factory& factory) {
        if (T* const node = free_.TryPop()) {
            return *node;
        }
        T& node = *new T(factory());
        all_.Push(node);
        return node;
    }

    /// Concurrent. The node must have been acquired from this storage.
    void Release(T& node) noexcept { free_.Push(node); }

    /// Concurrent. Visits the nodes published before traversal starts, acquired or not.
    /// The caller must synchronize access to node data.
    template <typename Func>
    void Walk(const Func& func) {
        all_.WalkUnsafe(func);
    }

private:
    using AllHook =
        concurrent::impl::CombinedHook<HookExtractor, concurrent::impl::MemberHook<&TimerStorageHook<T>::all_hook>>;
    using FreeHook =
        concurrent::impl::CombinedHook<HookExtractor, concurrent::impl::MemberHook<&TimerStorageHook<T>::free_hook>>;

    concurrent::impl::IntrusiveStack<T, AllHook> all_;
    concurrent::impl::IntrusiveStack<T, FreeHook> free_;
};

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END
