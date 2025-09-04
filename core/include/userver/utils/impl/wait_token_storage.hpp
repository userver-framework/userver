#pragma once

#include <cstdint>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

class WaitTokenStorageLock;
struct WaitTokenStorageImpl;

/// Gives out tokens and waits for all given-out tokens death.
///
/// The implementation is optimized for @ref GetToken efficiency. Waiting for remaining tokens in @ref WaitForAllTokens
/// may use extra CPU time.
class WaitTokenStorage final {
public:
    using Token = WaitTokenStorageLock;

    WaitTokenStorage();

    WaitTokenStorage(const WaitTokenStorage&) = delete;
    WaitTokenStorage(WaitTokenStorage&&) = delete;
    ~WaitTokenStorage();

    /// @brief Acquires a lock. While the lock is held, @ref WaitForAllTokens will not finish.
    ///
    /// It is OK to call `GetToken` in the following cases:
    /// * @ref WaitForAllTokens has not been called yet;
    /// * the caller owns another lock and guarantees that the @ref WaitForAllTokens call, if any, does not finish yet.
    ///
    /// Calling `GetToken` after @ref WaitForAllTokens has completed is UB.
    WaitTokenStorageLock GetToken();

    /// Approximate number of currently alive tokens
    std::uint64_t AliveTokensApprox() const noexcept;

    /// Wait until all given-out tokens are dead. Should be called at most once,
    /// either in a coroutine environment or after the coroutine environment
    /// stops (during static destruction).
    void WaitForAllTokens() noexcept;

private:
    friend class WaitTokenStorageLock;

    void DoLock();

    void DoUnlock() noexcept;

    impl::WaitTokenStorageImpl& impl_;
};

class WaitTokenStorageLock final {
public:
    /// @brief Produces a `null` instance.
    WaitTokenStorageLock() noexcept = default;

    /// @brief Locks `storage`. See @ref WaitTokenStorage::GetToken.
    explicit WaitTokenStorageLock(WaitTokenStorage& storage);

    WaitTokenStorageLock(WaitTokenStorageLock&&) noexcept;
    WaitTokenStorageLock(const WaitTokenStorageLock&);
    WaitTokenStorageLock& operator=(WaitTokenStorageLock&&) noexcept;
    WaitTokenStorageLock& operator=(const WaitTokenStorageLock&);
    ~WaitTokenStorageLock();

private:
    WaitTokenStorage* storage_{nullptr};
};

}  // namespace utils::impl

USERVER_NAMESPACE_END
