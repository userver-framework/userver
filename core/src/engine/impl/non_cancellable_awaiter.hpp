#include <userver/engine/impl/awaiter.hpp>

#include <utility>

#include <userver/engine/impl/context_accessor.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/make_intrusive_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

/// A simple awaiter implementation that does not support cancellations and does not need epoch/context.
template <typename CallbackType>
class NonCancellableAwaiter final : public PolymorphicAwaiter {
public:
    static_assert(std::is_nothrow_invocable_v<CallbackType&&>);

    explicit NonCancellableAwaiter(CallbackType&& callback)
        : callback_(std::move(callback))
    {}

    void NotifyAndDispose(std::uintptr_t context) noexcept override {
        UASSERT(context == 0);
        std::move(callback_)();
        delete this;
    }

    void DisposeWithoutNotification() noexcept override { delete this; }

private:
    [[no_unique_address]] CallbackType callback_;
};

// If Awaitable is already ready, then calls `callback` immediately.
// Callback should be able to run outside a coroutine.
template <typename CallbackType>
void AppendNonCancellableAwaiter(AwaitableBase& awaitable, CallbackType&& callback) {
    using AwaiterType = NonCancellableAwaiter<std::remove_const_t<std::remove_reference_t<CallbackType>>>;
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
    AwaiterPtr awaiter(new AwaiterType(std::forward<CallbackType>(callback)));
    awaitable.TryAppendAwaiter(awaiter, 0);
    if (awaiter != nullptr) {  // awaitable is ready.
        impl::NotifyAndDispose(std::move(awaiter), 0);
    }
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
