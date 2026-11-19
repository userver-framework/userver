#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <optional>
#include <string>

#include <userver/dist_lock/dist_lock_settings.hpp>
#include <userver/dist_lock/dist_lock_strategy.hpp>
#include <userver/dist_lock/statistics.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace dist_lock::impl {

enum class LockerMode {
    kOneshot,  ///< runs the payload once, then returns
    kWorker,   ///< reacquires lock and runs the payload indefinitely
};

class Locker final {
public:
    static constexpr DistLockRetryMode kDefaultRetryMode = DistLockRetryMode::kRetry;

    Locker(
        std::string name,
        std::shared_ptr<DistLockStrategyBase> strategy,
        const DistLockSettings& settings,
        std::function<void()> worker_func,
        DistLockRetryMode retry_mode = kDefaultRetryMode,
        logging::Level base_log_level = logging::Level::kInfo
    );

    const std::string& Name() const;
    const std::string& Id() const;

    DistLockSettings GetSettings() const;
    void SetSettings(const DistLockSettings&);

    std::optional<std::chrono::steady_clock::duration> GetLockedDuration() const;

    const Statistics& GetStatistics() const;

    engine::TaskWithResult<void> RunAsync(engine::TaskProcessor& task_processor, LockerMode, DistLockWaitingMode);

    bool OwnsLock() const noexcept;

    bool IsCancelAdvised() const;

private:
    class LockGuard;

    void ReleaseLock();

    void ReleaseLock(std::unique_lock<engine::Mutex>&& lock);

    void ResetFallback();

    /// Requires state_mutex_ to be locked.
    void ResetFallback(const std::lock_guard<engine::Mutex>& lock) noexcept;

    void Run(LockerMode, dist_lock::DistLockWaitingMode, tracing::Span&& span);

    /// @returns previous state
    bool ExchangeLockState(bool is_locked, std::chrono::steady_clock::time_point when);

    void RunWatchdog();

    void HandleBusyLock(engine::TaskWithResult<void> watchdog_task);

    LockOperation GetLockOperation(const DistLockSettings& settings, std::chrono::steady_clock::time_point now) const;

    LockOperationResult TryLockOperation(
        const DistLockSettings& settings,
        LockOperation operation,
        std::chrono::steady_clock::time_point now
    );

    LockOperationResult TryFallback(const DistLockSettings& settings);

    LockOperationResult CompleteAcquisition(
        const DistLockSettings& settings,
        LockOperation operation,
        std::chrono::steady_clock::time_point now
    );

    const std::string name_;
    const std::string id_;
    std::shared_ptr<DistLockStrategyBase> strategy_;
    std::function<void()> worker_func_;

    mutable engine::Mutex state_mutex_;
    DistLockSettings settings_;
    std::size_t negative_heartbeats_{0};
    const DistLockRetryMode retry_mode_;

    std::atomic<std::chrono::steady_clock::duration> fallback_until_{};

    std::atomic<bool> is_locked_{false};
    std::atomic<std::chrono::steady_clock::duration> lock_refresh_since_epoch_{};
    std::atomic<std::chrono::steady_clock::duration> lock_acquire_since_epoch_{};

    Statistics stats_;
    const logging::Level base_log_level_;
};

}  // namespace dist_lock::impl

USERVER_NAMESPACE_END
