#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <concurrent/impl/rseq.hpp>
#include <concurrent/impl/striped_array.hpp>
#include <userver/concurrent/impl/intrusive_hooks.hpp>
#include <userver/concurrent/impl/intrusive_stack.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

#if defined(USERVER_IMPL_HAS_RSEQ) || defined(DOXYGEN)

/// @brief A contention-free sharded atomic free list. Node objects are kept alive while in the list.
/// Nodes are not owned by the list.
///
/// Unless `USERVER_DISABLE_RSEQ_ACCELERATION` is set, uses per-CPU-core counters.
///
/// Can show emptiness if there are free nodes overall, but there are no nodes specifically for the current CPU core.
/// Because of this, `StripedIntrusivePool` usage can lead to up to `N_CORES` times memory consumption by nodes.
/// Do not use large objects as nodes!
///
/// `HookExtractor::GetHook` static function should get the hook subobject, given `T` node.
/// The hook's type must be `SinglyLinkedHook<T>`.
/// Additionally, `kHookOffset` must be `offsetof` the hook subobject within `T`.
template <typename T, typename HookExtractor, std::ptrdiff_t kHookOffset>
class StripedIntrusivePool final {
public:
    StripedIntrusivePool() = default;

    StripedIntrusivePool(StripedIntrusivePool&&) = delete;
    StripedIntrusivePool& operator=(StripedIntrusivePool&&) = delete;

    void Push(T& node) noexcept {
        // Implementation is taken from this_cpu_list_push in
        // https://github.com/compudj/librseq/blob/master/tests/basic_percpu_ops_test.c

        for (;;) {
            const int cpu = rseq_cpu_start();
            if (rseq_unlikely(!IsCpuIdValid(cpu))) {
                break;
            }

            auto& slot = array_[cpu];
            const auto expect = RSEQ_READ_ONCE(slot);  // Hacky atomic_ref load
            const auto newval = reinterpret_cast<std::intptr_t>(&node);
            UASSERT_MSG(
                // Unfortunately, there is no legal way to check this at compile-time.
                reinterpret_cast<std::byte*>(&GetNext(node)) - reinterpret_cast<std::byte*>(&node) == kHookOffset,
                "kHookOffset is invalid"
            );
            GetNext(node).store(reinterpret_cast<T*>(expect), std::memory_order_relaxed);

            const int ret = rseq_load_cbne_store__ptr(RSEQ_MO_RELAXED, RSEQ_PERCPU_CPU_ID, &slot, expect, newval, cpu);

            if (rseq_likely(!ret)) {
                return;
            }
            // Retry if rseq aborts.
        }

        fallback_list_.Push(node);
    }

    T* TryPop() noexcept {
        // Implementation is taken from this_cpu_list_pop in
        // https://github.com/compudj/librseq/blob/master/tests/basic_percpu_ops_test.c

        for (;;) {
            std::intptr_t head{};

            const int cpu = rseq_cpu_start();
            if (rseq_unlikely(!IsCpuIdValid(cpu))) {
                break;
            }

            auto& slot = array_[cpu];
            const auto expectnot = reinterpret_cast<std::intptr_t>(static_cast<T*>(nullptr));

            const int ret = rseq_load_cbeq_store_add_load_store__ptr(
                RSEQ_MO_RELAXED, RSEQ_PERCPU_CPU_ID, &slot, expectnot, kHookOffset, &head, cpu
            );

            if (rseq_likely(!ret)) {
                return reinterpret_cast<T*>(head);
            }
            if (ret > 0) {
                return nullptr;
            }
            // Retry if rseq aborts.
        }

        return fallback_list_.TryPop();
    }

    // Not thread-safe with respect to other methods.
    template <typename Func>
    void WalkUnsafe(const Func& func) {
        fallback_list_.WalkUnsafe(func);
        DoWalk<T&>(func);
    }

    // Not thread-safe with respect to other methods.
    template <typename Func>
    void WalkUnsafe(const Func& func) const {
        fallback_list_.WalkUnsafe(func);
        DoWalk<const T&>(func);
    }

    // Not thread-safe with respect to other methods.
    template <typename DisposerFunc>
    void DisposeUnsafe(const DisposerFunc& disposer) noexcept {
        fallback_list_.DisposeUnsafe(disposer);
        for (auto& slot : array_.Elements()) {
            T* iter = reinterpret_cast<T*>(slot);
            slot = reinterpret_cast<std::intptr_t>(static_cast<T*>(nullptr));
            while (iter) {
                T* const old_iter = iter;
                iter = GetNext(*iter).load();
                disposer(*old_iter);
            }
        }
    }

    // Not thread-safe with respect to other methods.
    std::size_t GetSizeUnsafe() const noexcept {
        std::size_t total_size = 0;
        WalkUnsafe([&total_size](const T&) { ++total_size; });
        return total_size;
    }

private:
    static std::atomic<T*>& GetNext(T& node) noexcept {
        SinglyLinkedHook<T>& hook = HookExtractor::GetHook(node);
        return hook.next;
    }

    template <typename U, typename Func>
    void DoWalk(const Func& func) const {
        for (auto& slot : array_.Elements()) {
            for (T* iter = reinterpret_cast<T*>(slot); iter; iter = GetNext(*iter).load()) {
                func(static_cast<U>(*iter));
            }
        }
    }

    impl::StripedArray array_;
    impl::IntrusiveStack<T, HookExtractor> fallback_list_;
};

#else

template <typename T, typename HookExtractor, std::ptrdiff_t kHookOffset>
using StripedIntrusivePool = impl::IntrusiveStack<T, HookExtractor>;

#endif

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
