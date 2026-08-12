#include <userver/utest/utest.hpp>

#include <chrono>
#include <string>
#include <string_view>

#include <fmt/format.h>

#include <userver/dist_lock/dist_lock_settings.hpp>
#include <userver/dist_lock/dist_lock_strategy.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/dist_lock_strategy.hpp>
#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/storages/postgres/utest/cluster_local.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;

using namespace std::chrono_literals;

namespace {

constexpr std::string_view kLockName = "lock";

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

}  // namespace

UTEST(PostgreDistLockStrategy, AcquireProlongRelease) {
    pg::utest::ClusterLocal local{};
    const auto& cluster = local.GetCluster();
    constexpr std::string_view kTable = "distlock_acquire_prolong_release";
    RecreateTable(cluster, kTable);

    pg::DistLockStrategy strategy{cluster, kTable, kLockName, dist_lock::DistLockSettings{}};
    UEXPECT_NO_THROW(strategy.Acquire(10s, "owner"));
    UEXPECT_NO_THROW(strategy.Prolong(10s, "owner"));
    UEXPECT_NO_THROW(strategy.Release("owner"));
}

UTEST(PostgreDistLockStrategy, ProlongExtendsTtl) {
    pg::utest::ClusterLocal local{};
    const auto& cluster = local.GetCluster();
    constexpr std::string_view kTable = "distlock_prolong_extends_ttl";
    RecreateTable(cluster, kTable);

    pg::DistLockStrategy strategy{cluster, kTable, kLockName, dist_lock::DistLockSettings{}};
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
    UEXPECT_THROW(strategy.Prolong(100s, "owner"), dist_lock::LockIsAcquiredByAnotherHostException);
}

USERVER_NAMESPACE_END
