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

    /// Releases the lock.
    ///
    /// @param locker_id Globally unique ID of the locking entity, must be the
    /// same as in Acquire().
    /// @note Exceptions are ignored.
    virtual void Release(const std::string& locker_id) = 0;
};

}  // namespace dist_lock

USERVER_NAMESPACE_END
