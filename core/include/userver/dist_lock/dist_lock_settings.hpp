#pragma once

/// @file userver/dist_lock/dist_lock_settings.hpp
/// @brief @copybrief dist_lock::DistLockSettings

#include <chrono>
#include <cstddef>
#include <optional>

USERVER_NAMESPACE_BEGIN

/// @brief Distributed locking
namespace dist_lock {

/// Fallback policy shared by all lock participants, including release history.
struct DistLockFallbackSettings {
    /// Consecutive successful checks that must find no live lock.
    std::size_t fallback_after_checks{50};

    /// How long a fallback host may acquire locks and continue its work.
    std::chrono::milliseconds fallback_duration{std::chrono::minutes{10}};

    /// Compare fallback policies.
    bool operator==(const DistLockFallbackSettings&) const = default;
};

/// Distributed lock settings
struct DistLockSettings {
    /// How often to try to acquire the lock.
    std::chrono::milliseconds acquire_interval{100};

    /// How often to try to prolong the lock while holding the lock.
    std::chrono::milliseconds prolong_interval{100};

    /// For how long to acquire/prolong the lock.
    std::chrono::milliseconds lock_ttl{1000};

    /// How much time we allow for the worker to stop when we're unable to prolong
    /// the lock.
    std::chrono::milliseconds forced_stop_margin{50};

    /// Delay before `DoWork` is restarted after exiting with or without an exception.
    std::chrono::milliseconds worker_func_restart_delay{100};

    /// Defaults to `true` and is updated from dynamic config.
    /// When switched to `false`, @ref dist_lock::DistLockedWorker will not start any new distributed lock acquisition
    /// attempts. If a `DoWork` invocation is already running under the lock, it is allowed to finish without forced
    /// cancellation. In this case @ref dist_lock::DistLockedWorker::IsCancelAdvised returns `true`.
    bool is_enabled{true};

    /// Shared fallback policy, including on preferred hosts that record release history.
    std::optional<DistLockFallbackSettings> fallback_policy{};

    /// If true, this host waits for fallback checks before acquiring the lock.
    /// After fallback succeeds, the host can acquire the lock normally until the fallback grant expires.
    /// If false, this host can acquire the lock immediately when the lock is available.
    bool is_host_deprioritized{false};

    /// Compare all timing, enablement and fallback settings.
    bool operator==(const DistLockSettings&) const = default;
};

/// Distributed lock waiting mode
enum class DistLockWaitingMode {
    kWait,    ///< waits until distlock becomes free
    kNoWait,  ///< doesn't wait for distlock if it is occupied
};

/// Distributed lock retry strategy
enum class DistLockRetryMode {
    kRetry,          ///< Retry on user exception/lock drop
    kSingleAttempt,  ///< Don't retry on user exception/lock drop
};

}  // namespace dist_lock

USERVER_NAMESPACE_END
