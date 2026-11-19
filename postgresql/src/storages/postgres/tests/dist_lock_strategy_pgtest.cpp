#include <userver/utest/utest.hpp>

#include <chrono>
#include <stdexcept>
#include <string>
#include <string_view>

#include <fmt/format.h>
#include <gmock/gmock.h>

#include <userver/dist_lock/dist_lock_settings.hpp>
#include <userver/dist_lock/dist_lock_strategy.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/dist_lock_strategy.hpp>
#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/storages/postgres/options.hpp>
#include <userver/storages/postgres/utest/cluster_local.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;

using namespace std::chrono_literals;

namespace {

constexpr std::string_view kLockName = "lock";

constexpr pg::CommandControl kLockCommandControl{
    std::chrono::seconds{2},
    std::chrono::milliseconds{500},
};

void RecreateTable(const pg::ClusterPtr& cluster, std::string_view table) {
    cluster->Execute(pg::ClusterHostType::kMaster, fmt::format("DROP TABLE IF EXISTS {}", table));
    cluster->Execute(
        pg::ClusterHostType::kMaster,
        fmt::format(
            "CREATE TABLE {} ("
            "key TEXT PRIMARY KEY, "
            "owner TEXT NOT NULL, "
            "expiration_time TIMESTAMPTZ NOT NULL)",
            table
        )
    );
}

pg::TimePointTz ReadExpiration(const pg::ClusterPtr& cluster, std::string_view table) {
    auto result = cluster->Execute(
        pg::ClusterHostType::kMaster,
        fmt::format("SELECT expiration_time FROM {} WHERE key = $1", table),
        std::string{kLockName}
    );
    return result.AsSingleRow<pg::TimePointTz>();
}

std::string ReadOwner(const pg::ClusterPtr& cluster, std::string_view table) {
    const auto result = cluster->Execute(
        pg::ClusterHostType::kMaster,
        fmt::format("SELECT owner FROM {} WHERE key = $1", table),
        std::string{kLockName}
    );
    return result.AsSingleRow<std::string>();
}

bool HasLockRow(const pg::ClusterPtr& cluster, std::string_view table) {
    const auto result = cluster->Execute(
        pg::ClusterHostType::kMaster,
        fmt::format("SELECT 1 FROM {} WHERE key = $1", table),
        std::string{kLockName}
    );
    return !result.IsEmpty();
}

void ExpireLock(const pg::ClusterPtr& cluster, std::string_view table) {
    cluster->Execute(
        pg::ClusterHostType::kMaster,
        fmt::format("UPDATE {} SET expiration_time = clock_timestamp() - INTERVAL '2 hours' WHERE key = $1", table),
        std::string{kLockName}
    );
}

}  // namespace

UTEST(PostgreDistLockStrategy, AcquireProlongRelease) {
    pg::utest::ClusterLocal local{};
    const auto& cluster = local.GetCluster();
    constexpr std::string_view kTable = "distlock_acquire_prolong_release";
    RecreateTable(cluster, kTable);

    pg::DistLockStrategy strategy{cluster, kTable, kLockName, dist_lock::DistLockSettings{}};
    strategy.UpdateCommandControl(kLockCommandControl);
    UEXPECT_NO_THROW(strategy.Acquire(10s, "owner"));
    UEXPECT_NO_THROW(strategy.Prolong(10s, "owner"));
    UEXPECT_NO_THROW(strategy.Release("owner"));
    EXPECT_FALSE(HasLockRow(cluster, kTable));
}

UTEST(PostgreDistLockStrategy, ProlongExtendsTtl) {
    pg::utest::ClusterLocal local{};
    const auto& cluster = local.GetCluster();
    constexpr std::string_view kTable = "distlock_prolong_extends_ttl";
    RecreateTable(cluster, kTable);

    pg::DistLockStrategy strategy{cluster, kTable, kLockName, dist_lock::DistLockSettings{}};
    strategy.UpdateCommandControl(kLockCommandControl);
    UEXPECT_NO_THROW(strategy.Acquire(1s, "owner"));
    const auto before = ReadExpiration(cluster, kTable);

    UEXPECT_NO_THROW(strategy.Prolong(3600s, "owner"));
    const auto after = ReadExpiration(cluster, kTable);

    EXPECT_LT(before.GetUnderlying(), after.GetUnderlying());
}

UTEST(PostgreDistLockStrategy, ProlongForeignOwnerThrows) {
    pg::utest::ClusterLocal local{};
    const auto& cluster = local.GetCluster();
    constexpr std::string_view kTable = "distlock_prolong_foreign_owner";
    RecreateTable(cluster, kTable);

    pg::DistLockStrategy strategy{cluster, kTable, kLockName, dist_lock::DistLockSettings{}};
    strategy.UpdateCommandControl(kLockCommandControl);
    UEXPECT_NO_THROW(strategy.Acquire(100s, "owner1"));
    const auto before = ReadExpiration(cluster, kTable);

    UEXPECT_THROW(strategy.Prolong(100s, "owner2"), dist_lock::LockIsAcquiredByAnotherHostException);

    const auto after = ReadExpiration(cluster, kTable);
    EXPECT_EQ(before.GetUnderlying(), after.GetUnderlying());
    UEXPECT_NO_THROW(strategy.Prolong(100s, "owner1"));
}

UTEST(PostgreDistLockStrategy, ProlongWithoutAcquireThrows) {
    pg::utest::ClusterLocal local{};
    const auto& cluster = local.GetCluster();
    constexpr std::string_view kTable = "distlock_prolong_without_acquire";
    RecreateTable(cluster, kTable);

    pg::DistLockStrategy strategy{cluster, kTable, kLockName, dist_lock::DistLockSettings{}};
    strategy.UpdateCommandControl(kLockCommandControl);
    UEXPECT_THROW(strategy.Prolong(100s, "owner"), dist_lock::LockIsAcquiredByAnotherHostException);
}

UTEST(PostgreDistLockStrategy, CheckDoesNotChangeLock) {
    pg::utest::ClusterLocal local{};
    const auto& cluster = local.GetCluster();
    constexpr std::string_view kTable = "distlock_check";
    RecreateTable(cluster, kTable);

    pg::DistLockStrategy strategy{cluster, kTable, kLockName, dist_lock::DistLockSettings{}};
    strategy.UpdateCommandControl(kLockCommandControl);
    const dist_lock::LockOperationOptions check{.lock_ttl = 10s, .operation = dist_lock::LockOperation::kCheck};

    EXPECT_EQ(strategy.ExecuteOperation("observer", check), dist_lock::LockOperationResult::kNotAlive);
    EXPECT_FALSE(HasLockRow(cluster, kTable));

    strategy.Acquire(10s, "owner");
    const auto live_expiration = ReadExpiration(cluster, kTable);
    EXPECT_EQ(strategy.ExecuteOperation("observer", check), dist_lock::LockOperationResult::kBusy);
    EXPECT_EQ(ReadExpiration(cluster, kTable).GetUnderlying(), live_expiration.GetUnderlying());

    ExpireLock(cluster, kTable);
    const auto expired_expiration = ReadExpiration(cluster, kTable);
    EXPECT_EQ(strategy.ExecuteOperation("observer", check), dist_lock::LockOperationResult::kNotAlive);
    EXPECT_EQ(ReadExpiration(cluster, kTable).GetUnderlying(), expired_expiration.GetUnderlying());
}

UTEST(PostgreDistLockStrategy, FallbackRequiresSharedIdleInterval) {
    pg::utest::ClusterLocal local{};
    const auto& cluster = local.GetCluster();
    constexpr std::string_view kTable = "distlock_fallback_idle_interval";
    RecreateTable(cluster, kTable);

    pg::DistLockStrategy strategy{cluster, kTable, kLockName, dist_lock::DistLockSettings{}};
    strategy.UpdateCommandControl(kLockCommandControl);
    const dist_lock::LockOperationOptions fallback{
        .lock_ttl = 10s,
        .operation = dist_lock::LockOperation::kFallback,
        .min_idle_time_milliseconds = 1h,
    };
    const dist_lock::LockOperationOptions check{.lock_ttl = 10s, .operation = dist_lock::LockOperation::kCheck};

    EXPECT_EQ(strategy.ExecuteOperation("fallback", fallback), dist_lock::LockOperationResult::kAcquired);
    EXPECT_EQ(strategy.ExecuteOperation("another-fallback", fallback), dist_lock::LockOperationResult::kBusy);
    strategy.Release("fallback", {.preserve_history = true});
    EXPECT_EQ(strategy.ExecuteOperation("fallback", check), dist_lock::LockOperationResult::kNotAlive);

    // A short preferred invocation between probes invalidates every contender's
    // earlier observations, including a contender that previously owned the lock.
    strategy.Acquire(10s, "preferred");
    strategy.Release("preferred", {.preserve_history = true});
    EXPECT_EQ(strategy.ExecuteOperation("fallback", check), dist_lock::LockOperationResult::kNotAlive);
    EXPECT_EQ(strategy.ExecuteOperation("fallback", fallback), dist_lock::LockOperationResult::kBusy);
    EXPECT_EQ(strategy.ExecuteOperation("preferred", fallback), dist_lock::LockOperationResult::kBusy);

    cluster->Execute(
        pg::ClusterHostType::kMaster,
        fmt::format("UPDATE {} SET expiration_time = clock_timestamp() - INTERVAL '2 minutes' WHERE key = $1", kTable),
        std::string{kLockName}
    );
    EXPECT_EQ(strategy.ExecuteOperation("fallback", fallback), dist_lock::LockOperationResult::kBusy);

    ExpireLock(cluster, kTable);
    EXPECT_EQ(strategy.ExecuteOperation("fallback", fallback), dist_lock::LockOperationResult::kAcquired);
}

UTEST(PostgreDistLockStrategy, FallbackAcceptsSubsecondIdleInterval) {
    pg::utest::ClusterLocal local{};
    const auto& cluster = local.GetCluster();
    constexpr std::string_view kTable = "distlock_fallback_subsecond_idle_interval";
    RecreateTable(cluster, kTable);

    pg::DistLockStrategy strategy{cluster, kTable, kLockName, dist_lock::DistLockSettings{}};
    strategy.UpdateCommandControl(kLockCommandControl);
    strategy.Acquire(10s, "preferred");
    strategy.Release("preferred", {.preserve_history = true});
    cluster->Execute(
        pg::ClusterHostType::kMaster,
        fmt::format("UPDATE {} SET expiration_time = clock_timestamp() - INTERVAL '2 seconds' WHERE key = $1", kTable),
        std::string{kLockName}
    );

    const dist_lock::LockOperationOptions fallback{
        .lock_ttl = 10s,
        .operation = dist_lock::LockOperation::kFallback,
        .min_idle_time_milliseconds = 500ms,
    };
    EXPECT_EQ(strategy.ExecuteOperation("fallback", fallback), dist_lock::LockOperationResult::kAcquired);
}

UTEST(PostgreDistLockStrategy, FallbackWithInvalidIdleIntervalThrows) {
    pg::utest::ClusterLocal local{};
    const auto& cluster = local.GetCluster();
    constexpr std::string_view kTable = "distlock_fallback_invalid_idle_interval";
    RecreateTable(cluster, kTable);

    pg::DistLockStrategy strategy{cluster, kTable, kLockName, dist_lock::DistLockSettings{}};
    strategy.UpdateCommandControl(kLockCommandControl);
    const dist_lock::LockOperationOptions invalid_fallback{
        .lock_ttl = 10s,
        .operation = dist_lock::LockOperation::kFallback,
        .min_idle_time_milliseconds = -1h,
    };

    UEXPECT_THROW(strategy.ExecuteOperation("fallback", invalid_fallback), std::invalid_argument);
    EXPECT_FALSE(HasLockRow(cluster, kTable));
}

UTEST(PostgreDistLockStrategy, PreservedHistoryCannotBeProlongedOrReleasedByAnotherOwner) {
    pg::utest::ClusterLocal local{};
    const auto& cluster = local.GetCluster();
    constexpr std::string_view kTable = "distlock_release_history";
    RecreateTable(cluster, kTable);

    pg::DistLockStrategy strategy{cluster, kTable, kLockName, dist_lock::DistLockSettings{}};
    strategy.UpdateCommandControl(kLockCommandControl);
    strategy.Acquire(10s, "owner");
    strategy.Release("owner", {.preserve_history = true});
    EXPECT_THAT(ReadOwner(cluster, kTable), ::testing::IsEmpty());
    const auto release_time = ReadExpiration(cluster, kTable);

    UEXPECT_THROW(strategy.Prolong(10s, "owner"), dist_lock::LockIsAcquiredByAnotherHostException);
    EXPECT_EQ(ReadExpiration(cluster, kTable).GetUnderlying(), release_time.GetUnderlying());
    EXPECT_THAT(ReadOwner(cluster, kTable), ::testing::IsEmpty());

    strategy.Acquire(10s, "other");
    const auto current_owner = ReadOwner(cluster, kTable);
    const auto current_expiration = ReadExpiration(cluster, kTable);

    // A repeated release from the previous owner must fail after the next owner
    // acquires the lock, even when both use the same strategy and hostname.
    UEXPECT_THROW(
        strategy.Release("owner", {.preserve_history = true}),
        dist_lock::LockIsAcquiredByAnotherHostException
    );
    EXPECT_EQ(ReadOwner(cluster, kTable), current_owner);
    EXPECT_EQ(ReadExpiration(cluster, kTable).GetUnderlying(), current_expiration.GetUnderlying());
}

UTEST(PostgreDistLockStrategy, LateDeleteCannotRemoveNewOwner) {
    pg::utest::ClusterLocal local{};
    const auto& cluster = local.GetCluster();
    constexpr std::string_view kTable = "distlock_late_delete";
    RecreateTable(cluster, kTable);

    pg::DistLockStrategy strategy{cluster, kTable, kLockName, dist_lock::DistLockSettings{}};
    strategy.UpdateCommandControl(kLockCommandControl);
    strategy.Acquire(10s, "previous");
    strategy.Release("previous");
    strategy.Acquire(10s, "current");
    const auto current_owner = ReadOwner(cluster, kTable);
    const auto current_expiration = ReadExpiration(cluster, kTable);

    UEXPECT_THROW(strategy.Release("previous"), dist_lock::LockIsAcquiredByAnotherHostException);
    EXPECT_EQ(ReadOwner(cluster, kTable), current_owner);
    EXPECT_EQ(ReadExpiration(cluster, kTable).GetUnderlying(), current_expiration.GetUnderlying());
}

USERVER_NAMESPACE_END
