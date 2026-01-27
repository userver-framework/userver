#pragma once

#include <condition_variable>

#include <userver/utils/assert.hpp>

#if __has_include(<linux/futex.h>)

#include <linux/futex.h> /* Definition of FUTEX_* constants */
#include <sys/syscall.h> /* Definition of SYS_* constants */
#include <unistd.h>

#include <limits>

#endif

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

class WaitWakeCondvar final {
public:
    WaitWakeCondvar() = default;

    std::size_t WakeupAll() {
        {
            const std::lock_guard guard{mutex_};
        }
        condvar_.notify_all();
        return std::numeric_limits<std::size_t>::max();
    }

    std::size_t WakeupSome(int /*wakeup_at_most*/) {
        WakeupAll();
        return std::numeric_limits<std::size_t>::max();
    }

    // May wake up more than one waiter due to internal limitations
    std::size_t WakeupByIndex(std::size_t /*index*/) {
        WakeupAll();
        return std::numeric_limits<std::size_t>::max();
    }

    template <class Predicate>
    void WaitByIndex(std::size_t /*index*/, Predicate pred) {
        std::unique_lock lock{mutex_};
        condvar_.wait(lock, std::move(pred));
    }

private:
    std::mutex mutex_{};
    std::condition_variable condvar_{};
};

#if __has_include(<linux/futex.h>)

class WaitWakeFutex final {
public:
    WaitWakeFutex() = default;

    std::size_t WakeupAll() { return WakeupByBitmask(FUTEX_BITSET_MATCH_ANY, kAllBitsetWaiters); }

    // Returns number of woken up waiters that were sleeping in OS (may return less than actually woke up)
    std::size_t WakeupSome(int wakeup_at_most) { return WakeupByBitmask(FUTEX_BITSET_MATCH_ANY, wakeup_at_most); }

    // May wake up more than one waiter due to internal limitations (may return less than actually woke up)
    std::size_t WakeupByIndex(std::size_t index) { return WakeupByBitmask(IndexToBitmask(index), kAllBitsetWaiters); }

    template <class Predicate>
    void WaitByIndex(std::size_t index, Predicate pred) {
        const auto bitmask = IndexToBitmask(index);

        for (;;) {
            std::uint32_t snapshot;  // NOLINT(cppcoreguidelines-init-variables)
            __atomic_load(&generation_, &snapshot, __ATOMIC_SEQ_CST);
            if (pred()) {
                break;
            }

            const auto ret = syscall(SYS_futex, &generation_, FUTEX_WAIT_BITSET, snapshot, NULL, NULL, bitmask);
            UINVARIANT(ret != -1 || errno == EAGAIN || errno == EWOULDBLOCK, "Failure in futex(FUTEX_WAIT_BITSET)");
        }
    }

private:
    static constexpr int kAllBitsetWaiters = std::numeric_limits<int>::max();
    static constexpr int IndexToBitmask(std::size_t index) { return static_cast<int>(1 << (index % kBitsInBitset)); }

    std::size_t WakeupByBitmask(int bitmask, int wakeup_at_most) {
        __atomic_add_fetch(&generation_, 1, __ATOMIC_SEQ_CST);

        const auto ret = syscall(SYS_futex, &generation_, FUTEX_WAKE_BITSET, wakeup_at_most, NULL, NULL, bitmask);
        UINVARIANT(ret != -1 || errno == EAGAIN || errno == EWOULDBLOCK, "Failure in futex(FUTEX_WAKE_BITSET)");
        return ret;
    }

    static constexpr int kBitsInBitset = 32;

    std::uint32_t generation_{0};
};

using WaitWake = WaitWakeFutex;

#else

using WaitWake = WaitWakeCondvar;

#endif

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
