#pragma once

/// @file userver/dist_lock/dist_lock_strategy.hpp
/// @brief @copybrief dist_lock::DistLockStrategyBase

#include <chrono>
#include <exception>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace dist_lock {

/// Indicates that lock cannot be acquired because it's busy.
class LockIsAcquiredByAnotherHostException : public std::exception {};

/// Operation performed by @ref dist_lock::DistLockStrategyBase::ExecuteOperation.
enum class LockOperation {
    kAcquire,   ///< Acquire an available lock.
    kProlong,   ///< Refresh a lock already owned by this locker.
    kCheck,     ///< Check for a live lock without acquiring or refreshing it.
    kFallback,  ///< Acquire only after a shared interval without lock activity.
};

/// Expected outcome of a distributed lock operation. Backend failures still throw.
enum class LockOperationResult {
    kAcquired,     ///< The lock was acquired or refreshed.
    kBusy,         ///< The lock is busy or the required idle interval has not elapsed.
    kNotAlive,     ///< A successful check found no live lock.
    kUnsupported,  ///< The strategy does not support this operation.
};

/// Parameters for @ref dist_lock::DistLockStrategyBase::ExecuteOperation.
struct LockOperationOptions {
    /// Duration for which the lock is acquired or refreshed.
    std::chrono::milliseconds lock_ttl;

    /// Acquisition, refresh, or a read-only liveness check.
    LockOperation operation{LockOperation::kAcquire};

    /// Nonnegative shared interval without activity required by @ref dist_lock::LockOperation::kFallback.
    std::chrono::milliseconds min_idle_time_milliseconds{};
};

/// Parameters for @ref dist_lock::DistLockStrategyBase::Release.
struct ReleaseOptions {
    /// Preserve the release timestamp for a shared fallback policy. Every lock
    /// participant, including preferred hosts, must use the same setting.
    bool preserve_history{false};
};

/// @ingroup userver_base_classes userver_concurrency
///
/// @brief Interface for distributed lock strategies
///
/// ## Example
///
/// @snippet core/src/dist_lock/dist_lock_test.cpp Sample distlock strategy
class DistLockStrategyBase {
public:
    virtual ~DistLockStrategyBase() = default;

    /// Acquires the distributed lock.
    ///
    /// @param lock_ttl The duration for which the lock must be held.
    /// @param locker_id Globally unique ID of the locking entity.
    /// @throws LockIsAcquiredByAnotherHostError when the lock is busy
    /// @throws anything else when the locking fails, strategy is responsible for
    /// cleanup, Release won't be invoked.
    virtual void Acquire(std::chrono::milliseconds lock_ttl, const std::string& locker_id) = 0;

    /// Prolongs (refreshes the TTL of) a lock already held by @a locker_id.
    ///
    /// Called instead of Acquire() on every refresh while the lock is held, so
    /// backends may implement a cheaper query than the full Acquire().
    ///
    /// @param lock_ttl The new duration for which the lock must be held.
    /// @param locker_id Globally unique ID of the locking entity, must be the
    /// same as in Acquire().
    /// @throws LockIsAcquiredByAnotherHostException when the lock is no longer
    /// held by @a locker_id (ownership was lost)
    /// @throws anything else when the prolongation fails, strategy is responsible
    /// for cleanup, Release won't be invoked.
    /// @note The default implementation simply calls Acquire(), preserving the
    /// legacy behaviour for strategies that do not override it.
    virtual void Prolong(std::chrono::milliseconds lock_ttl, const std::string& locker_id) {
        Acquire(lock_ttl, locker_id);
    }

    /// Acquire, refresh, or check a lock, returning expected outcomes without exceptions.
    /// The default implementation delegates acquisition and refresh to the legacy
    /// virtual methods. Checks and fallback acquisition are unsupported by default,
    /// so enabling fallback cannot accidentally acquire a lock while checking it.
    /// Backend errors other than contention are propagated to the caller.
    virtual LockOperationResult ExecuteOperation(const std::string& locker_id, const LockOperationOptions& options) {
        try {
            switch (options.operation) {
                case LockOperation::kAcquire:
                    Acquire(options.lock_ttl, locker_id);
                    return LockOperationResult::kAcquired;
                case LockOperation::kProlong:
                    Prolong(options.lock_ttl, locker_id);
                    return LockOperationResult::kAcquired;
                case LockOperation::kCheck:
                case LockOperation::kFallback:
                    return LockOperationResult::kUnsupported;
            }
        } catch (const LockIsAcquiredByAnotherHostException&) {
            return LockOperationResult::kBusy;
        }
        return LockOperationResult::kUnsupported;
    }

    /// Releases the lock.
    ///
    /// @param locker_id Globally unique ID of the locking entity, must be the
    /// same as in Acquire().
    /// @note Exceptions are ignored.
    virtual void Release(const std::string& locker_id) = 0;

    /// Release with the current shared fallback policy. The default implementation
    /// delegates to the legacy release method; legacy strategies do not support
    /// fallback acquisition. Backends supporting fallback must preserve history
    /// when requested, and must only release the lock owned by @a locker_id.
    virtual void Release(const std::string& locker_id, const ReleaseOptions&) { Release(locker_id); }
};

}  // namespace dist_lock

USERVER_NAMESPACE_END
