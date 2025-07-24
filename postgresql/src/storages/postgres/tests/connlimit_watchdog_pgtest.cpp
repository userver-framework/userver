#include <storages/postgres/connlimit_watchdog.hpp>

#include <storages/postgres/tests/util_pgtest.hpp>

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <algorithm>

#include <userver/utest/utest.hpp>

#include <storages/postgres/detail/cluster_impl.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/postgres_config.hpp>
#include <userver/utils/statistics/metrics_storage.hpp>

#include <userver/dynamic_config/test_helpers.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;
namespace pgd = storages::postgres::detail;

namespace {

constexpr std::size_t kShardNumber = 0;

pgd::ClusterImpl CreateClusterImpl(
    const pg::DsnList& dsns,
    engine::TaskProcessor& bg_task_processor,
    size_t max_size,
    testsuite::TestsuiteTasks& testsuite_tasks,
    pg::ConnectionSettings conn_settings = kCachePreparedStatements
) {
    auto source = dynamic_config::GetDefaultSource();
    return pgd::ClusterImpl(
        dsns,
        nullptr,
        bg_task_processor,
        {{},
         {utest::kMaxTestWaitTime},
         {0, max_size, max_size},
         conn_settings,
         pg::InitMode::kAsync,
         "",
         pg::ConnlimitMode::kAuto,
         {}},
        {kTestCmdCtl, {}, {}},
        {},
        {},
        testsuite_tasks,
        source,
        std::make_shared<USERVER_NAMESPACE::utils::statistics::MetricsStorage>(),
        kShardNumber
    );
}

const std::string kWatchdogTaskName = fmt::format("connlimit_watchdog_{}_{}", "", kShardNumber);

constexpr std::string_view kRawInsert = R"(
        INSERT INTO u_clients (hostname, updated, max_connections, cur_user) VALUES
        ($1, NOW(), $2, {}) ON CONFLICT (hostname) DO UPDATE SET updated = NOW(), max_connections = $2, cur_user = {}
    )";

constexpr std::size_t kHostsCount = 10;

pg::Transaction GetTransaction(pgd::ClusterImpl& cluster) {
    static pg::CommandControl kCommandControl{std::chrono::seconds(2), std::chrono::seconds(2)};
    return cluster.Begin({pg::ClusterHostType::kMaster}, {}, kCommandControl);
}

constexpr size_t kReservedConn = 5;
constexpr size_t kTestsuiteConnlimit = 100 - kReservedConn;

enum class MigrationVersion { V1 = 0, V2 = 1, Count };

}  // namespace

class Watchdog : public PostgreSQLBase {
public:
    static_assert(
        static_cast<int>(MigrationVersion::Count) == 2,
        "It is very dangerous. You must add new tests for a new migration version!"
    );

    Watchdog()
        : cluster_(
              CreateClusterImpl(this->GetDsnListFromEnv(), this->GetTaskProcessor(), kHostsCount * 2, testsuite_tasks_)
          ) {
        // Do the step of ConnlimitWatchdog to create the table.
        testsuite_tasks_.RunTask(kWatchdogTaskName);

        ClearTable();
    }

    std::size_t DoStepV1() {
        // This watchdog use the native host like the watchdog in ClusterImpl.
        pg::ConnlimitWatchdog connlimit_watchdog_V1{cluster_, testsuite_tasks_, kShardNumber, [] {}};
        connlimit_watchdog_V1.StepV1();
        return connlimit_watchdog_V1.GetConnlimit();
    }

    std::size_t DoStepV2() {
        // Use different host names to emulate different hosts.
        pg::ConnlimitWatchdog connlimit_watchdog_V2{cluster_, testsuite_tasks_, kShardNumber, [] {}, "host2"};
        connlimit_watchdog_V2.StepV2();
        return connlimit_watchdog_V2.GetConnlimit();
    }

    pgd::ClusterImpl& GetCluster() { return cluster_; }

private:
    void ClearTable() {
        auto t = GetTransaction(cluster_);
        t.Execute("DELETE FROM u_clients");
        t.Commit();
    }

private:
    testsuite::TestsuiteTasks testsuite_tasks_{true};
    pgd::ClusterImpl cluster_;
};

UTEST_F(Watchdog, Basic) {
    EXPECT_EQ(kTestsuiteConnlimit, DoStepV1());

    // There are two hosts after 'StepV2'.
    EXPECT_EQ(kTestsuiteConnlimit / 2, DoStepV2());

    // There are two hosts after 'StepV2'.
    EXPECT_EQ(kTestsuiteConnlimit / 2, DoStepV1());
}

#ifdef __cpp_concepts
template <class T>
concept HasNewVersion = requires { &T::StepV3; };
static_assert(
    !HasNewVersion<pg::ConnlimitWatchdog>,
    "Please update the following test for StepV* and increment the version check in above concept"
);
#endif
// We check different combinations of queries order with table 'u_clients', because
// services can be deployed on different versions of userver and rolled back to random version
UTEST_F(Watchdog, AllPermutations) {
    static_assert(
        (&pg::ConnlimitWatchdog::StepV1) && (&pg::ConnlimitWatchdog::StepV2),
        "Do not remove old versions of StepV*, because there may be users that still use it and they may update "
        "userver one day. So we need to make sure that the update (and a rollback) will be successfull."
    );
    // Fill the table by two rows.
    EXPECT_EQ(kTestsuiteConnlimit, DoStepV1());
    EXPECT_EQ(kTestsuiteConnlimit / 2, DoStepV2());

    std::vector<MigrationVersion> combinations{
        MigrationVersion::V1, MigrationVersion::V1, MigrationVersion::V2, MigrationVersion::V2};
    auto do_step = [this](MigrationVersion version) {
        if (version == MigrationVersion::V1) {
            EXPECT_EQ(kTestsuiteConnlimit / 2, DoStepV1());
        } else if (version == MigrationVersion::V2) {
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

USERVER_NAMESPACE_END
