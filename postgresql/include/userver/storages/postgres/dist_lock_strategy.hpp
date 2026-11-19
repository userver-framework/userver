#pragma once

/// @file userver/storages/postgres/dist_lock_strategy.hpp
/// @brief @copybrief storages::postgres::DistLockStrategy

#include <string_view>

#include <userver/dist_lock/dist_lock_settings.hpp>
#include <userver/dist_lock/dist_lock_strategy.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/storages/postgres/options.hpp>
#include <userver/storages/postgres/query.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

/// Postgres distributed locking strategy.
/// Release history, when requested, retains one expired row per lock key to
/// track the shared idle interval used by fallback acquisition.
class DistLockStrategy final : public dist_lock::DistLockStrategyBase {
public:
    DistLockStrategy(
        ClusterPtr cluster,
        std::string_view table,
        std::string_view lock_name,
        const dist_lock::DistLockSettings& settings
    );

    void Acquire(std::chrono::milliseconds lock_ttl, const std::string& locker_id) override;

    /// @brief Acquires, prolongs or checks the lock according to @a options.
    /// A check never changes the lock. Fallback acquisition requires the shared
    /// lock row to have been inactive for the entire minimum idle interval.
    /// @throws std::invalid_argument if the minimum idle interval is negative.
    dist_lock::LockOperationResult ExecuteOperation(
        const std::string& locker_id,
        const dist_lock::LockOperationOptions& options
    ) override;

    void Prolong(std::chrono::milliseconds lock_ttl, const std::string& locker_id) override;

    /// @brief Releases the lock by deleting its row.
    /// @throws dist_lock::LockIsAcquiredByAnotherHostException if this locker no longer owns the lock.
    void Release(const std::string& locker_id) override;

    /// @brief Releases the lock according to the shared fallback policy.
    /// When @ref dist_lock::ReleaseOptions::preserve_history is enabled,
    /// retains the release time in an expired row instead of deleting the row.
    /// Retained history prevents fallback contenders from reusing observations
    /// made before another worker acquired and released the lock. Ordinary
    /// acquisition can reuse the released row immediately.
    /// @throws dist_lock::LockIsAcquiredByAnotherHostException if this locker no longer owns the lock.
    void Release(const std::string& locker_id, const dist_lock::ReleaseOptions& options) override;

    void UpdateCommandControl(CommandControl cc);

private:
    dist_lock::LockOperationResult RunLockQuery(
        std::string_view locker_id,
        const dist_lock::LockOperationOptions& options
    );

    dist_lock::LockOperationResult CheckLock();

    ClusterPtr cluster_;
    rcu::Variable<CommandControl> cc_;
    const Query acquire_query_;
    const Query fallback_acquire_query_;
    const Query check_query_;
    const Query prolong_query_;
    const Query release_query_;
    const Query release_with_history_query_;
    const std::string lock_name_;
    const std::string owner_prefix_;
};

}  // namespace storages::postgres

USERVER_NAMESPACE_END
