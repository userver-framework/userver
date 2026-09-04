#include <storages/postgres/connlimit_watchdog.hpp>

#include <storages/postgres/tests/util_pgtest.hpp>

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <utility>

#include <userver/engine/deadline.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/impl/userver_experiments.hpp>
#include <userver/utils/statistics/metrics_storage.hpp>

#include <storages/postgres/detail/cluster_impl.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/postgres_config.hpp>

#include <userver/dynamic_config/test_helpers.hpp>

#include <dynamic_config/variables/POSTGRES_CONNLIMIT_MODE_AUTO_ENABLED.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;
namespace pgd = storages::postgres::detail;

namespace {

constexpr std::size_t kShardNumber = 0;
constexpr std::size_t kConfiguredMinPoolSize = 4;
constexpr std::size_t kConfiguredMaxPoolSize = 20;
const std::string kWatchdogTaskName = fmt::format("connlimit_watchdog_{}_{}", "", kShardNumber);

pgd::ClusterImpl CreateClusterImpl(
    const pg::DsnList& dsns,
    engine::TaskProcessor& bg_task_processor,
    testsuite::TestsuiteTasks& testsuite_tasks,
    dynamic_config::Source config_source,
    pg::ConnectionSettings conn_settings = kCachePreparedStatements,
    pg::InitMode init_mode = pg::InitMode::kAsync
) {
    return pgd::ClusterImpl(
        dsns,
        nullptr,
        bg_task_processor,
        {{},
         {utest::kMaxTestWaitTime},
         {kConfiguredMinPoolSize, kConfiguredMaxPoolSize, kConfiguredMaxPoolSize},
         conn_settings,
         init_mode,
         "",
         pg::ConnlimitMode::kAuto,
         {}},
        {kTestCmdCtl, {}, {}},
        {},
        {},
        testsuite_tasks,
        std::move(config_source),
        std::make_shared<USERVER_NAMESPACE::utils::statistics::MetricsStorage>(),
        kShardNumber
    );
}

pgd::ClusterImpl CreateClusterImplForStartupTest(
    const pg::DsnList& dsns,
    engine::TaskProcessor& bg_task_processor,
    testsuite::TestsuiteTasks& testsuite_tasks,
    dynamic_config::Source config_source,
    std::size_t min_pool_size,
    std::size_t max_pool_size,
    pg::ConnlimitMode connlimit_mode
) {
    return pgd::ClusterImpl(
        dsns,
        nullptr,
        bg_task_processor,
        {{},
         {utest::kMaxTestWaitTime},
         {min_pool_size, max_pool_size, max_pool_size},
         kCachePreparedStatements,
         pg::InitMode::kSync,
         "",
         connlimit_mode,
         {}},
        {kTestCmdCtl, {}, {}},
        {},
        {},
        testsuite_tasks,
        std::move(config_source),
        std::make_shared<USERVER_NAMESPACE::utils::statistics::MetricsStorage>(),
        kShardNumber
    );
}

constexpr std::string_view kRawInsert = R"(
        INSERT INTO u_clients (hostname, updated, max_connections, cur_user) VALUES
        ($1, NOW(), $2, {}) ON CONFLICT (hostname) DO UPDATE SET updated = NOW(), max_connections = $2, cur_user = {}
    )";

pg::Transaction GetTransaction(pgd::ClusterImpl& cluster) {
    static pg::CommandControl command_control{std::chrono::seconds(2), std::chrono::seconds(2)};
    return cluster.Begin({pg::ClusterHostType::kMaster}, {}, command_control);
}

void ClearWatchdogTable(pgd::ClusterImpl& cluster) {
    auto trx = GetTransaction(cluster);
    trx.Execute("DELETE FROM u_clients");
    trx.Commit();
}

void WaitForActiveConnections(pgd::ClusterImpl& cluster, std::size_t expected) {
    const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
    while (!deadline.IsReached()) {
        if (cluster.GetStatistics()->master.stats.connection.active == expected) {
            return;
        }
        engine::SleepFor(std::chrono::milliseconds{20});
    }

    FAIL() << "Timed out waiting for " << expected << " active PostgreSQL connections";
}

constexpr size_t kReservedConn = 5;
constexpr size_t kTestsuiteServerConnlimit = 100;
constexpr size_t kTestsuiteConnlimit = kTestsuiteServerConnlimit - kReservedConn;
constexpr size_t kFallbackConnlimit = 17;
constexpr size_t kMaxStepsWithError = 3;

enum class MigrationVersion { kV1 = 0, kV2 = 1, kCount };

}  // namespace

class Watchdog : public PostgreSQLBase {
public:
    static_assert(
        static_cast<int>(MigrationVersion::kCount) == 2,
        "It is very dangerous. You must add new tests for a new migration version!"
    );

    Watchdog()
        : cluster_(
              CreateClusterImpl(GetDsnListFromEnv(), GetTaskProcessor(), testsuite_tasks_, config_storage_.GetSource())
          )
    {
        ClearWatchdogTable(cluster_);
    }

    std::size_t DoStepV1() {
        // This watchdog use the native host like the watchdog in ClusterImpl.
        auto connlimit_watchdog_v1 = MakeConnlimitWatchdog();
        connlimit_watchdog_v1.StepV1();
        return connlimit_watchdog_v1.GetConnlimit();
    }

    std::size_t DoStepV2() {
        // Use different host names to emulate different hosts.
        auto connlimit_watchdog_v2 = MakeConnlimitWatchdog("host2");
        connlimit_watchdog_v2.StepV2();
        return connlimit_watchdog_v2.GetConnlimit();
    }

    pg::ConnlimitWatchdog MakeConnlimitWatchdog(
        std::string host_name = hostinfo::blocking::GetRealHostName(),
        std::size_t non_pool_connections_per_instance = 0
    ) {
        return pg::ConnlimitWatchdog{
            cluster_,
            testsuite_tasks_,
            kShardNumber,
            kFallbackConnlimit,
            [] {},
            non_pool_connections_per_instance,
            host_name,
        };
    }

    pgd::ClusterImpl& GetCluster() { return cluster_; }

    void RunWatchdogStep() { testsuite_tasks_.RunTask(kWatchdogTaskName); }

private:
    dynamic_config::StorageMock config_storage_{
        dynamic_config::MakeDefaultStorage({{::dynamic_config::POSTGRES_CONNLIMIT_MODE_AUTO_ENABLED, true}})
    };
    testsuite::TestsuiteTasks testsuite_tasks_{true};
    pgd::ClusterImpl cluster_;
    utils::impl::UserverExperimentsScope scope_;
};

template <class T>
concept HasNewVersion = requires { &T::StepV3; };

template <class T>
concept HasOldVersions = requires {
    &T::StepV1;
    &T::StepV2;
};

static_assert(
    !HasNewVersion<pg::ConnlimitWatchdog>,
    "Please update the following test for StepV* and increment the version check in above concept"
);

// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class WatchdogWarmup : public PostgreSQLBase, public ::testing::WithParamInterface<pg::InitMode> {};

UTEST_P(WatchdogWarmup, BootstrapsAndWarmsUpAfterRegistration) {
    dynamic_config::StorageMock config_storage{
        dynamic_config::MakeDefaultStorage({{::dynamic_config::POSTGRES_CONNLIMIT_MODE_AUTO_ENABLED, true}})
    };
    testsuite::TestsuiteTasks testsuite_tasks{true};
    utils::impl::UserverExperimentsScope experiments_scope;
    auto cluster = CreateClusterImpl(
        GetDsnListFromEnv(),
        GetTaskProcessor(),
        testsuite_tasks,
        config_storage.GetSource(),
        kCachePreparedStatements,
        GetParam()
    );
    ClearWatchdogTable(cluster);

    {
        const auto statistics = cluster.GetStatistics();
        EXPECT_EQ(1, statistics->master.stats.connection.maximum);
        EXPECT_LE(statistics->master.stats.connection.active, 1);
    }

    testsuite_tasks.RunTask(kWatchdogTaskName);
    WaitForActiveConnections(cluster, kConfiguredMinPoolSize);

    const auto statistics = cluster.GetStatistics();
    EXPECT_EQ(kTestsuiteConnlimit, statistics->master.stats.connection.maximum);
    EXPECT_EQ(kConfiguredMinPoolSize, statistics->master.stats.connection.active);
}

INSTANTIATE_UTEST_SUITE_P(
    InitModes,
    WatchdogWarmup,
    ::testing::Values(pg::InitMode::kSync, pg::InitMode::kAsync),
    [](const testing::TestParamInfo<WatchdogWarmup::ParamType>& info) {
        return info.param == pg::InitMode::kSync ? "Sync" : "Async";
    }
);

UTEST_F(Watchdog, ZeroConnlimitDoesNotDisablePool) {
    GetCluster().SetDsnList({GetUnavailableDsn()});
    EXPECT_NO_THROW(RunWatchdogStep());

    const auto statistics = GetCluster().GetStatistics();
    EXPECT_EQ(1, statistics->master.stats.connection.maximum);
}

// We check different combinations of queries order with table 'u_clients', because
// services can be deployed on different versions of userver and rolled back to random version
UTEST_F(Watchdog, AllPermutations) {
    static_assert(
        HasOldVersions<pg::ConnlimitWatchdog>,
        "Do not remove old versions of StepV*, because there may be users that still use it and they may update "
        "userver one day. So we need to make sure that the update (and a rollback) will be successful."
    );
    // Fill the table by two rows.
    EXPECT_EQ(kTestsuiteConnlimit, DoStepV1());
    EXPECT_EQ(kTestsuiteConnlimit / 2, DoStepV2());

    std::vector<MigrationVersion>
        combinations{MigrationVersion::kV1, MigrationVersion::kV1, MigrationVersion::kV2, MigrationVersion::kV2};
    auto do_step = [this](MigrationVersion version) {
        if (version == MigrationVersion::kV1) {
            EXPECT_EQ(kTestsuiteConnlimit / 2, DoStepV1());
        } else if (version == MigrationVersion::kV2) {
            EXPECT_EQ(kTestsuiteConnlimit / 2, DoStepV2());
        } else {
            UINVARIANT(false, "Please provide the code for this version");
        }
    };

    do {
        for (const auto version : combinations) {
            do_step(version);
        }
    } while (std::next_permutation(combinations.begin(), combinations.end()));
}

UTEST_F(Watchdog, MultiUsersWithV1) {
    DoStepV1();
    {
        auto t = GetTransaction(GetCluster());
        const auto user_name = R"('new_user')";
        t.Execute(fmt::format(kRawInsert, user_name, user_name), "new_user_host1", 7);
        t.Commit();
    }
    // StepV1 divides connections between all users => 'new_user' affects a connlimit.
    EXPECT_EQ(kTestsuiteConnlimit / 2, DoStepV1());
    {
        auto t = GetTransaction(GetCluster());
        const auto user_name = R"('new_user')";
        t.Execute(fmt::format(kRawInsert, user_name, user_name), "new_user_host2", 7);
        t.Commit();
    }
    // StepV1 divides connections between all users => 'new_user' affects a connlimit.
    EXPECT_EQ(kTestsuiteConnlimit / 3, DoStepV1());
}

UTEST_F(Watchdog, MultiUsersWithV2) {
    DoStepV2();
    {
        auto t = GetTransaction(GetCluster());
        const auto user_name = R"('new_user')";
        t.Execute(fmt::format(kRawInsert, user_name, user_name), "new_user_host1", 7);
        t.Commit();
    }
    // StepV2 divides connections only between current_user => 'new_user' doesn't affect a connlimit.
    EXPECT_EQ(kTestsuiteConnlimit, DoStepV2());
    {
        auto t = GetTransaction(GetCluster());
        const auto user_name = R"('new_user')";
        t.Execute(fmt::format(kRawInsert, user_name, user_name), "new_user_host2", 7);
        t.Commit();
    }
    EXPECT_EQ(kTestsuiteConnlimit, DoStepV2());
    {
        auto t = GetTransaction(GetCluster());
        const auto user_name = "current_user";
        // Insert the second host of 'current_user' => connlimit := connlimit / 2
        t.Execute(fmt::format(kRawInsert, user_name, user_name), "new_current_user_host", 7);
        t.Commit();
    }
    // StepV2 divides connections only between current_user => new host of 'current_user' affects a connlimit.
    EXPECT_EQ(kTestsuiteConnlimit / 2, DoStepV2());
}

UTEST_F(Watchdog, FallbackConnlimit) {
    auto expected_connlimit = kTestsuiteConnlimit;
    auto watchdog = MakeConnlimitWatchdog();
    // Do single step with working connection
    watchdog.StepV2();
    // Update connection to a non-working one
    GetCluster().SetDsnList({GetUnavailableDsn()});

    while (expected_connlimit >= kFallbackConnlimit) {
        for (size_t i = 0; i <= kMaxStepsWithError; ++i) {
            ASSERT_EQ(expected_connlimit, watchdog.GetConnlimit());
            watchdog.StepV2();
        }
        expected_connlimit /= 2;
    }

    ASSERT_EQ(kFallbackConnlimit, watchdog.GetConnlimit());
}

UTEST_F(Watchdog, CheckLimit) {
    constexpr auto
        kConnectionsLimit = kTestsuiteServerConnlimit - static_cast<std::size_t>(kTestsuiteServerConnlimit * 0.05);

    EXPECT_EQ(kConnectionsLimit, DoStepV1());

    // There are two hosts after 'StepV2'.
    EXPECT_EQ(kConnectionsLimit / 2, DoStepV2());

    // There are two hosts after 'StepV2'.
    EXPECT_EQ(kConnectionsLimit / 2, DoStepV1());
}

UTEST_F(Watchdog, AccountsForNonPoolConnections) {
    constexpr std::size_t kTopologyConnectionsPerInstance = 1;
    auto first_watchdog = MakeConnlimitWatchdog("host-with-topology-connection-1", kTopologyConnectionsPerInstance);
    first_watchdog.StepV2();
    EXPECT_EQ(kTestsuiteConnlimit - kTopologyConnectionsPerInstance, first_watchdog.GetConnlimit());

    auto second_watchdog = MakeConnlimitWatchdog("host-with-topology-connection-2", kTopologyConnectionsPerInstance);
    second_watchdog.StepV2();

    EXPECT_EQ(kTestsuiteConnlimit / 2 - kTopologyConnectionsPerInstance, second_watchdog.GetConnlimit());
}

UTEST_F(Watchdog, AutoModeDoesNotExhaustConnectionsOnSimultaneousStart) {
    constexpr std::size_t kStartupMinPoolSize = kTestsuiteConnlimit;

    auto config_storage = dynamic_config::MakeDefaultStorage({
        {::dynamic_config::POSTGRES_CONNLIMIT_MODE_AUTO_ENABLED, true},
    });
    testsuite::TestsuiteTasks first_tasks{true};
    testsuite::TestsuiteTasks second_tasks{true};
    testsuite::TestsuiteTasks third_tasks{true};
    auto first_cluster = CreateClusterImplForStartupTest(
        GetDsnListFromEnv(),
        GetTaskProcessor(),
        first_tasks,
        config_storage.GetSource(),
        kStartupMinPoolSize,
        kStartupMinPoolSize,
        pg::ConnlimitMode::kAuto
    );
    auto second_cluster = CreateClusterImplForStartupTest(
        GetDsnListFromEnv(),
        GetTaskProcessor(),
        second_tasks,
        config_storage.GetSource(),
        kStartupMinPoolSize,
        kStartupMinPoolSize,
        pg::ConnlimitMode::kAuto
    );
    auto third_cluster = CreateClusterImplForStartupTest(
        GetDsnListFromEnv(),
        GetTaskProcessor(),
        third_tasks,
        config_storage.GetSource(),
        kStartupMinPoolSize,
        kStartupMinPoolSize,
        pg::ConnlimitMode::kAuto
    );

    const std::array clusters{&first_cluster, &second_cluster, &third_cluster};
    const std::array tasks{&first_tasks, &second_tasks, &third_tasks};
    for (const auto* cluster : clusters) {
        const auto statistics = cluster->GetStatistics();
        EXPECT_EQ(statistics->master.stats.connection.maximum, 1);
        EXPECT_LE(statistics->master.stats.connection.active, 1);
    }

    for (std::size_t i = 0; i < clusters.size(); ++i) {
        auto watchdog = pg::ConnlimitWatchdog{
            *clusters[i],
            *tasks[i],
            kShardNumber,
            kStartupMinPoolSize,
            [] {},
            0,
            fmt::format("simultaneous-start-host-{}", i),
        };
        watchdog.StepV2();
        EXPECT_GT(watchdog.GetConnlimit(), 0);
    }

    auto trx = GetTransaction(GetCluster());
    EXPECT_EQ(trx.Execute("SELECT count(*) FROM u_clients").AsSingleRow<int>(), static_cast<int>(clusters.size()));
}

UTEST_F(Watchdog, AutoModeStartsFromReservedConnectionsBelowMinPoolSize) {
    constexpr std::size_t kStartupMinPoolSize = 10;

    const auto already_open_connections = GetCluster().GetStatistics()->master.stats.connection.active;
    ASSERT_LT(already_open_connections, kTestsuiteConnlimit);

    testsuite::TestsuiteTasks occupying_tasks{true};
    auto occupying_cluster = CreateClusterImplForStartupTest(
        GetDsnListFromEnv(),
        GetTaskProcessor(),
        occupying_tasks,
        dynamic_config::GetDefaultSource(),
        kTestsuiteConnlimit - already_open_connections,
        kTestsuiteConnlimit - already_open_connections,
        pg::ConnlimitMode::kManual
    );
    EXPECT_EQ(
        occupying_cluster.GetStatistics()->master.stats.connection.active,
        kTestsuiteConnlimit - already_open_connections
    );

    auto config_storage = dynamic_config::MakeDefaultStorage({
        {::dynamic_config::POSTGRES_CONNLIMIT_MODE_AUTO_ENABLED, true},
    });
    testsuite::TestsuiteTasks new_instance_tasks{true};
    auto new_instance = CreateClusterImplForStartupTest(
        GetDsnListFromEnv(),
        GetTaskProcessor(),
        new_instance_tasks,
        config_storage.GetSource(),
        kStartupMinPoolSize,
        kStartupMinPoolSize,
        pg::ConnlimitMode::kAuto
    );

    {
        const auto statistics = new_instance.GetStatistics();
        EXPECT_EQ(statistics->master.stats.connection.maximum, 1);
        EXPECT_LE(statistics->master.stats.connection.active, 1);
    }

    auto watchdog = pg::ConnlimitWatchdog{
        new_instance,
        new_instance_tasks,
        kShardNumber,
        kStartupMinPoolSize,
        [] {},
        0,
        "reserved-connection-host",
    };
    watchdog.StepV2();
    EXPECT_GT(watchdog.GetConnlimit(), 0);

    auto trx = GetTransaction(GetCluster());
    EXPECT_EQ(
        trx.Execute("SELECT count(*) FROM u_clients WHERE hostname = 'reserved-connection-host'").AsSingleRow<int>(),
        1
    );
}

USERVER_NAMESPACE_END
