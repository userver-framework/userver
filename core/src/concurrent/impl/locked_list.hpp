#pragma once

#include <atomic>
#include <cstdint>

#ifndef __cpp_lib_atomic_ref
#include <boost/atomic/atomic_ref.hpp>
#endif

#include <compiler/relax_cpu.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

#ifdef __cpp_lib_atomic_ref
namespace locked_list_atomics = std;
#else
namespace locked_list_atomics = boost;
#endif

template <typename T>
struct DoublyLinkedHook {
    T* prev{nullptr};
    T* next{nullptr};
};

// Non-owning intrusive list. Callers synchronize access to the nodes and their hooks.
template <typename T, typename HookExtractor>
class DoublyLinkedList final {
public:
    explicit DoublyLinkedList(T* head = nullptr) noexcept : head_(head) {}
    DoublyLinkedList(const DoublyLinkedList&) = delete;
    DoublyLinkedList& operator=(const DoublyLinkedList&) = delete;

    T* GetHead() const noexcept { return head_; }

    bool IsEmpty() const noexcept { return head_ == nullptr; }

    void PushFront(T& node) noexcept {
        auto& hook = GetHook(node);
        UASSERT(!hook.prev && !hook.next && head_ != &node);
        hook.next = head_;
        if (head_) {
            GetHook(*head_).prev = &node;
        }
        head_ = &node;
    }

    void Erase(T& node) noexcept {
        auto& hook = GetHook(node);
        if (hook.prev) {
            GetHook(*hook.prev).next = hook.next;
        } else {
            UASSERT(head_ == &node);
            head_ = hook.next;
        }
        if (hook.next) {
            GetHook(*hook.next).prev = hook.prev;
        }
        hook = {};
    }

    T& PopFront() noexcept {
        UASSERT(!IsEmpty());
        auto& node = *head_;
        auto& hook = GetHook(node);
        head_ = hook.next;
        if (head_) {
            GetHook(*head_).prev = nullptr;
            hook.next = nullptr;
        }
        return node;
    }

private:
    static DoublyLinkedHook<T>& GetHook(T& node) noexcept { return HookExtractor::GetHook(node); }

    T* head_;
};

// An unlocked shard stores the list head (zero for an empty list).
// The guard exchanges the head for kLocked on acquisition and stores the updated head back on release.
template <typename T, typename HookExtractor>
class LockedList final {
public:
    explicit LockedList(std::intptr_t& shard) noexcept : shard_(shard), list_(Lock()) {}

    LockedList(const LockedList&) = delete;
    LockedList& operator=(const LockedList&) = delete;
    ~LockedList() {
        shard_.store(reinterpret_cast<std::intptr_t>(list_.GetHead()), locked_list_atomics::memory_order_release);
    }

    DoublyLinkedList<T, HookExtractor>& GetList() & noexcept { return list_; }

private:
    // Assumes no list node resides at this address.
    static constexpr std::intptr_t kLocked = 1;
    static_assert(alignof(std::intptr_t) >= locked_list_atomics::atomic_ref<std::intptr_t>::required_alignment);

    T* Lock() noexcept {
        compiler::RelaxCpu relax;
        for (;;) {
            const auto head = shard_.exchange(kLocked, locked_list_atomics::memory_order_acquire);
            if (head != kLocked) {
                return reinterpret_cast<T*>(head);
            }
            while (shard_.load(locked_list_atomics::memory_order_relaxed) == kLocked) {
                relax();
            }
        }
    }

    locked_list_atomics::atomic_ref<std::intptr_t> shard_;
    DoublyLinkedList<T, HookExtractor> list_;
};

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
