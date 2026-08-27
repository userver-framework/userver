#pragma once

/// @file userver/concurrent/impl/intrusive_thread_unsafe_slist.hpp
/// @brief Single thread helpers to manage and iterate over concurrent::impl::SinglyLinkedBaseHook

#include <cstdint>
#include <memory>
#include <type_traits>

#include <userver/concurrent/impl/intrusive_hooks.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

template <class T>
class ThreadUnsafeSlistIterator;

template <class T, class Deleter = std::default_delete<T>>
class ThreadUnsafeSlist final {
public:
    using iterator = ThreadUnsafeSlistIterator<T>;

    constexpr ThreadUnsafeSlist() = default;

    ThreadUnsafeSlist(ThreadUnsafeSlist&&) = delete;
    ThreadUnsafeSlist& operator=(ThreadUnsafeSlist&&) = delete;

    ~ThreadUnsafeSlist() { EraseFromBegin(iterator{}); }

    iterator Adopt(iterator prev, SinglyLinkedBaseHook* new_node) noexcept {
        auto* previous_node = prev.GetNodeRawPointer();
        if (empty()) {
            UASSERT(prev == end());
            previous_node = &nodes_;
        }
        UASSERT(previous_node);

        previous_node->singly_linked_hook.next.store(new_node, std::memory_order_relaxed);  // Used in single thread
        return iterator{new_node};
    }

    void EraseFromBegin(iterator end) noexcept {
        iterator it = begin();
        while (it != end) {
            auto* ptr = std::addressof(*it);
            ++it;
            Deleter{}(ptr);
        }

        nodes_.singly_linked_hook.next.store(end.GetNodeRawPointer(), std::memory_order_relaxed);
    }

    inline iterator begin() noexcept {
        return iterator{nodes_.singly_linked_hook.next.load(std::memory_order_relaxed)};
    }

    inline iterator end() noexcept { return iterator{}; }

    [[nodiscard]] bool empty() const noexcept {
        return nodes_.singly_linked_hook.next.load(std::memory_order_relaxed) == nullptr;
    }

private:
    SinglyLinkedBaseHook nodes_;
};

template <class T>
class ThreadUnsafeSlistIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using reference = value_type&;
    using pointer = value_type*;

    ThreadUnsafeSlistIterator() = default;

    constexpr explicit ThreadUnsafeSlistIterator(SinglyLinkedBaseHook* node) noexcept
      : node_(node)
    {}

    constexpr ThreadUnsafeSlistIterator& operator++() noexcept {
        UASSERT(node_);
        node_ = node_->singly_linked_hook.next.load(std::memory_order_relaxed);  // Used in single thread
        return *this;
    }

    constexpr ThreadUnsafeSlistIterator operator++(int) noexcept {
        UASSERT(node_);
        auto copy = *this;
        ++(*this);
        return copy;
    }

    T& operator*() const noexcept {
        UASSERT(node_);

        static_assert(std::derived_from<T, SinglyLinkedBaseHook>);
        return static_cast<T&>(*node_);  // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
    }

    T* operator->() const noexcept { return &**this; }

    bool operator==(const ThreadUnsafeSlistIterator&) const noexcept = default;

    SinglyLinkedBaseHook* GetNodeRawPointer() noexcept { return node_; }

private:
    SinglyLinkedBaseHook* node_ = nullptr;
};

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
