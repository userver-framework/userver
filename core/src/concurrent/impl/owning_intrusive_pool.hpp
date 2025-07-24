#pragma once

#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

// Wraps a non-owning concurrent intrusive pool (stack, queue, does not matter)
// into an owning pool of heap-allocated nodes.
//
// Requires the following from `NonOwningPoolType`:
// * the default constructor;
// * `T* TryPop()`
// * `void Push(T&)`
// * `void DisposeUnsafe(auto callback) noexcept`
template <typename NonOwningPoolType>
class OwningIntrusivePool final {
public:
    using ItemType = std::remove_pointer_t<decltype(std::declval<NonOwningPoolType&>().TryPop())>;

    constexpr OwningIntrusivePool() = default;

    OwningIntrusivePool(OwningIntrusivePool&&) = delete;
    OwningIntrusivePool& operator=(OwningIntrusivePool&&) = delete;

    ~OwningIntrusivePool() {
        impl_.DisposeUnsafe([](ItemType& item) { delete &item; });
    }

    ItemType& Acquire() {
        return Acquire([] { return ItemType(); });
    }

    template <typename Factory>
    auto& Acquire(const Factory& factory) {
        if (ItemType* node = impl_.TryPop()) {
            return *node;
        }
        return *new ItemType(factory());
    }

    void Release(ItemType& node) { impl_.Push(node); }

private:
    NonOwningPoolType impl_;
};

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
