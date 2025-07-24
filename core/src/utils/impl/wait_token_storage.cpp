#include <userver/utils/impl/wait_token_storage.hpp>

#include <algorithm>  // for std::max
#include <atomic>
#include <mutex>
#include <utility>

#include <concurrent/impl/owning_intrusive_pool.hpp>
#include <userver/compiler/impl/constexpr.hpp>
#include <userver/concurrent/impl/asymmetric_fence.hpp>
#include <userver/concurrent/impl/intrusive_hooks.hpp>
#include <userver/concurrent/impl/intrusive_stack.hpp>
#include <userver/concurrent/impl/striped_read_indicator.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

struct WaitTokenStorageImpl final {
    // Given that WaitTokenStorageImpl is reused and there can be racy DoUnlock calls from a previous reincarnation
    // of `*this`, this event can be sent spuriously. Check `tokens.IsFree()`.
    engine::SingleConsumerEvent tokens_is_free_event;
    concurrent::impl::StripedReadIndicator tokens;
    concurrent::impl::SinglyLinkedHook<WaitTokenStorageImpl> pool_hook;
    std::mutex shutdown_mutex;
    std::atomic<bool> shutdown_started{false};
};

namespace {

using WaitTokenStorageImplPool = concurrent::impl::OwningIntrusivePool<concurrent::impl::IntrusiveStack<
    WaitTokenStorageImpl,
    concurrent::impl::MemberHook<&WaitTokenStorageImpl::pool_hook>>>;

USERVER_IMPL_CONSTINIT WaitTokenStorageImplPool wait_token_storage_impl_pool;

}  // namespace

WaitTokenStorage::WaitTokenStorage() : impl_(wait_token_storage_impl_pool.Acquire()) {
    // Reinitialize impl_.
    UASSERT(impl_.tokens.IsFree());
    impl_.shutdown_started.store(false, std::memory_order_relaxed);

    // Lock the initial token so that if token count reaches 0 before shutdown starts,
    // tokens_is_free_event.Send() is not called.
    impl_.tokens.lock();
}

WaitTokenStorage::~WaitTokenStorage() {
    if (!impl_.shutdown_started.load()) {
        impl_.tokens.unlock();
    }

    if (!impl_.tokens.IsFree()) {
        // WaitForAllTokens has not been called (e.g. an exception has been thrown
        // from WaitTokenStorage owner's constructor), and there are some tokens
        // still alive. Don't wait for them, because that can cause a hard-to-detect deadlock.
        AbortWithStacktrace("Some tokens are still alive while the WaitTokenStorage is being destroyed");
    }

    wait_token_storage_impl_pool.Release(impl_);
}

WaitTokenStorageLock WaitTokenStorage::GetToken() { return WaitTokenStorageLock{*this}; }

std::uint64_t WaitTokenStorage::AliveTokensApprox() const noexcept {
    return std::max(impl_.tokens.GetActiveCountUpperEstimate(), std::uintptr_t{1}) - 1;
}

void WaitTokenStorage::WaitForAllTokens() noexcept {
    if (impl_.shutdown_started.load()) {
        UASSERT_MSG(false, "WaitForAllTokens must be called at most once");
        return;
    }

    const bool skip_waiting =
        // WaitTokenStorage is being destroyed outside of coroutine context, typically during static destruction.
        // In this case, we should have already waited for all tasks when exiting the coroutine context.
        // If new tokens have been taken, we can't wait for them at this point.
        !engine::current_task::IsTaskProcessorThread() ||
        // Optimistic path. See IsFree guarantees. Note that taking first lock during WaitForAllTokens is UB by design.
        impl_.tokens.GetActiveCountUpperEstimate() == 1;

    if (skip_waiting) {
        impl_.shutdown_started.store(true);
        impl_.tokens.unlock();
        return;
    }

    {
        const std::lock_guard lock{impl_.shutdown_mutex};

        impl_.shutdown_started.store(true, std::memory_order_seq_cst);

        // To make sure all tokens_.lock_shared() that saw `shutdown_started == false` reach us according to total
        // order. New DoUnlock calls will synchronize using `shutdown_mutex` instead.
        concurrent::impl::AsymmetricThreadFenceHeavy();

        impl_.tokens.unlock();  // std::memory_order_release
    }

    if (impl_.tokens.IsFree()) {
        impl_.tokens_is_free_event.Send();
    }

    const engine::TaskCancellationBlocker cancel_blocker;
    const bool wait_success =
        impl_.tokens_is_free_event.WaitUntil(engine::Deadline{}, [this] { return impl_.tokens.IsFree(); });
    UASSERT(wait_success);
}

void WaitTokenStorage::DoLock() {
    UASSERT_MSG(!impl_.tokens.IsFree(), "WaitForAllTokens has already completed");

    impl_.tokens.lock();  // std::memory_order_relaxed

    // To make sure WaitForAllTokens sees our relaxed counter increment and propagates it to other unlock_shared calls.
    concurrent::impl::AsymmetricThreadFenceLight();
}

void WaitTokenStorage::DoUnlock() noexcept {
    auto& impl = impl_;

    // Immediately after this call, a parallel WaitForAllTokens call can detect `IsFree() == true` and destroy
    // the WaitTokenStorage. But the code below still needs it. One solution would be to pin `impl`
    // using a hazard pointer. Since there are no hazard pointers in userver at the time of writing, we instead "leak"
    // `impl` to a global pool in `~WaitTokenStorage`.
    impl.tokens.unlock();  // std::memory_order_release

    // Makes sure that if `shutdown_started.load() -> false` then `impl.tokens.unlock()` will reach WaitForAllTokens.
    concurrent::impl::AsymmetricThreadFenceLight();

    if (impl.shutdown_started.load(std::memory_order_seq_cst)) {
        // This mutex lock serves two purposes:
        // 1. AsymmetricThreadFenceHeavy in WaitForAllTokens is guaranteed to happen-before our mutex lock.
        //    Thus, all `impl.tokens.unlock()` calls with `shutdown_started == false` happen-before.
        // 2. Of all `tokens` unlockers with `shutdown_started == true`, including WaitForAllTokens,
        //    there will be the last one to acquire `shutdown_mutex` lock. It will see all the previous
        //    `impl.tokens.unlock()` calls due to acquire-release ordering in `std::mutex` lock-unlock.
        { const std::lock_guard lock{impl.shutdown_mutex}; }

        if (impl.tokens.IsFree()) {
            impl.tokens_is_free_event.Send();
        }
    }
}

WaitTokenStorageLock::WaitTokenStorageLock(WaitTokenStorage& storage) : storage_(&storage) { storage_->DoLock(); }

WaitTokenStorageLock::WaitTokenStorageLock(WaitTokenStorageLock&& other) noexcept
    : storage_(std::exchange(other.storage_, nullptr)) {}

WaitTokenStorageLock::WaitTokenStorageLock(const WaitTokenStorageLock& other) : storage_(other.storage_) {
    if (storage_ != nullptr) {
        storage_->DoLock();
    }
}

WaitTokenStorageLock& WaitTokenStorageLock::operator=(WaitTokenStorageLock&& other) noexcept {
    if (&other != this) {
        if (storage_ != nullptr) {
            storage_->DoUnlock();
        }
        storage_ = std::exchange(other.storage_, nullptr);
    }
    return *this;
}

WaitTokenStorageLock& WaitTokenStorageLock::operator=(const WaitTokenStorageLock& other) {
    *this = WaitTokenStorageLock{other};
    return *this;
}

WaitTokenStorageLock::~WaitTokenStorageLock() {
    if (storage_ != nullptr) {
        storage_->DoUnlock();
    }
}

}  // namespace utils::impl

USERVER_NAMESPACE_END
