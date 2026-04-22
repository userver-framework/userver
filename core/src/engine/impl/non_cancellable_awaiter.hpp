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
        : PolymorphicAwaiter(Awaiter::InitialRefCounter::kOne),
          callback_(std::move(callback))
    {}

    void DoNotify(boost::intrusive_ptr<PolymorphicAwaiter> self, std::uintptr_t context) noexcept override {
        UASSERT(context == 0);
        std::move(callback_)();

        UASSERT(self->UseCount() == 1);
        // Avoid an extra atomic refcount decrement.
        delete static_cast<NonCancellableAwaiter*>(self.detach());
    }

    void Destroy() noexcept override { delete this; }

private:
    CallbackType callback_;
};

// If ContextAccessor is already ready, then calls `callback` immediately.
// Callback should be able to run outside a coroutine.
template <typename CallbackType>
void AppendNonCancellableAwaiter(ContextAccessor& awaitable, CallbackType&& callback) {
    using AwaiterType = NonCancellableAwaiter<std::remove_const_t<std::remove_reference_t<CallbackType>>>;
    boost::intrusive_ptr<Awaiter> awaiter{new AwaiterType(std::forward<CallbackType>(callback)), /*add_ref=*/false};
    awaitable.TryAppendAwaiter(awaiter, 0);
    if (awaiter != nullptr) {  // awaitable is ready.
        impl::Notify(std::move(awaiter), 0);
    }
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
