#pragma once

#include <cstdint>
#include <limits>

#include <userver/concurrent/striped_counter.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

class StripedReadIndicatorLock;

/// @ingroup userver_concurrency
///
/// @brief Protects some data from being modified or deleted as long as there is
/// at least one reader.
///
/// Under heavy concurrent usage, performs far better than something like
/// a refcount, @ref lock and @ref unlock are wait-free population oblivious.
///
/// Allocates `16 * N_CORES` bytes, which is more than a simple refcount,
/// of course. Use sparingly and beware of the memory usage.
///
/// Another drawback compared to a conventional refcount is that free-ness is
/// not signaled directly. The only way to await `IsFree` is to check it
/// periodically.
///
/// @see Based on ideas from https://youtu.be/FtaD0maxwec
class StripedReadIndicator final {
public:
    /// @brief Create a new unused instance of `ReadIndicator`.
    StripedReadIndicator();

    StripedReadIndicator(StripedReadIndicator&&) = delete;
    StripedReadIndicator& operator=(StripedReadIndicator&&) = delete;
    ~StripedReadIndicator();

    /// @brief Acquire a lock on the indicator, marking it "used" until the lock is destroyed.
    /// @note The lock should not outlive the `StripedReadIndicator`.
    /// @see @ref StripedReadIndicator::lock for concurrency guarantees.
    StripedReadIndicatorLock GetLock() noexcept;

    /// @brief Acquire a lock on the indicator, marking it "used" until @ref unlock is called.
    /// @warning Use @ref GetLock instead of manual lock-unlock to avoid leaking locks!
    /// @note The lock should not outlive the `StripedReadIndicator`.
    /// @note The data may still be retired in parallel with a `lock` call.
    /// After calling `lock`, the reader must check whether the data has been
    /// retired in the meantime.
    /// @note Uses `std::memory_order_relaxed`.
    /// @warning Readers must ensure that `lock` is visible by `IsFree` checks
    /// in other threads when necessary
    /// (which may require `std::atomic_thread_fence(std::memory_order_seq_cst)`.
    /// `lock` and `IsFree` use weak memory orders, which may lead to `lock`
    /// unexpectedly not being visible to `IsFree`.
    void lock() noexcept;

    /// @brief Same as @ref lock, to satisfy standard `Mutex` concept.
    [[nodiscard]] bool try_lock() noexcept {
        lock();
        return true;
    }

    /// @brief Remove a previously acquired lock.
    /// @warning Use `std::unique_lock` instead of manual lock-unlock to avoid leaking locks!
    /// @note Uses `std::memory_order_release` to ensure that unlocks don't run ahead
    /// of locks from `IsFree`'s point of view.
    void unlock() noexcept;

    /// @returns `true` if there are no locks held on the `StripedReadIndicator`.
    /// @note `IsFree` should only be called after direct access to this
    /// StripedReadIndicator is closed for readers. Locks acquired during
    /// the `IsFree` call may or may not be accounted for.
    /// @note May sometimes falsely return `false` when the `StripedReadIndicator`
    /// has just become free, then became locked again. Never falsely returns `true`.
    /// @note Uses effectively `std::memory_order_acquire`.
    /// It means that results of `IsFree`, @ref lock and @ref unlock may contradict
    /// total order, but will still follow happens-before.
    bool IsFree() const noexcept;

    /// @returns `true` if there are no locks held on any of the `indicators`.
    /// @see IsFree
    template <typename StripedReadIndicatorRange>
    static bool AreAllFree(StripedReadIndicatorRange&& indicators) {
        // See GetActiveCountUpperEstimate for implementation strategy explanation.
        std::uintptr_t released = 0;
        for (const auto& indicator : indicators) {
            released += indicator.released_count_.Read();
        }

        std::uintptr_t acquired = 0;
        for (const auto& indicator : indicators) {
            acquired += indicator.acquired_count_.Read();
        }

        UASSERT(acquired - released <= std::numeric_limits<std::uintptr_t>::max() / 2);
        return acquired == released;
    }

    /// Get the total amount of @ref lock calls, useful for metrics.
    std::uintptr_t GetAcquireCountApprox() const noexcept;

    /// Get the total amount of @ref unlock calls, useful for metrics.
    std::uintptr_t GetReleaseCountApprox() const noexcept;

    /// Similarly to @ref IsFree, returns the upper estimate of the number of locks held.
    /// May show more locks than the actual count. Never shows fewer locks than the actual count.
    std::uintptr_t GetActiveCountUpperEstimate() const noexcept;

private:
    friend class StripedReadIndicatorLock;

    StripedCounter acquired_count_;
    StripedCounter released_count_;
};

/// @brief Keeps the data protected by a StripedReadIndicator from being retired.
class [[nodiscard]] StripedReadIndicatorLock final {
public:
    /// @brief Produces a `null` instance.
    StripedReadIndicatorLock() noexcept = default;

    /// @brief Locks `indicator`. See @ref StripedReadIndicator::lock.
    explicit StripedReadIndicatorLock(StripedReadIndicator&& indicator) noexcept;

    StripedReadIndicatorLock(StripedReadIndicatorLock&&) noexcept;
    StripedReadIndicatorLock(const StripedReadIndicatorLock&) noexcept;
    StripedReadIndicatorLock& operator=(StripedReadIndicatorLock&&) noexcept;
    StripedReadIndicatorLock& operator=(const StripedReadIndicatorLock&) noexcept;
    ~StripedReadIndicatorLock();

private:
    explicit StripedReadIndicatorLock(StripedReadIndicator& indicator) noexcept;

    friend class StripedReadIndicator;

    StripedReadIndicator* indicator_{nullptr};
};

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
