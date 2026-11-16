#pragma once

#include <atomic>
#include <mutex>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

class AsymmetricFenceCoalescer final {
public:
    template <typename Func>
    bool TryPerform(const Func& perform) noexcept(noexcept(perform())) {
        const std::unique_lock lock{mutex_, std::try_to_lock};
        if (!lock.owns_lock()) {
            return false;
        }
        perform();
        return true;
    }

    template <typename Func>
    void Perform(const Func& perform) noexcept(noexcept(perform())) {
        const std::lock_guard lock{mutex_};
        perform();
    }

    void Enqueue() noexcept {
        // RMWs keep all coalesced requests in the release sequence read by
        // PerformPending. A store could overwrite an earlier caller's release,
        // leaving its preceding operations unordered with the heavy fence.
        pending_.exchange(true, std::memory_order_seq_cst);
    }

    template <typename Func>
    void PerformPending(const Func& perform) noexcept(noexcept(perform())) {
        const std::lock_guard lock{mutex_};
        // If the flag is clear, another callback has already consumed the queued
        // requests; the mutex ensures its barrier has finished. Requests arriving
        // after this exchange need another barrier.
        if (pending_.exchange(false, std::memory_order_seq_cst)) {
            perform();
        }
    }

private:
    std::mutex mutex_;
    std::atomic<bool> pending_{false};
};

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
