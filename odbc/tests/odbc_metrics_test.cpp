#include <userver/utest/utest.hpp>

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <storages/odbc/detail/statement_stats.hpp>
#include <storages/odbc/detail/statement_stats_storage.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/storages/odbc.hpp>
#include <userver/storages/odbc/tests/utils.hpp>

#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/statistics/testing.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::tests {

namespace {

using namespace std::chrono_literals;

bool WaitForMetric(
    const utils::statistics::Storage& storage,
    std::string path,
    std::vector<utils::statistics::Label> labels
) {
    for (std::size_t i = 0; i < 100; ++i) {
        const utils::statistics::Snapshot snapshot{storage, "odbc", {{"odbc_pool", "0"}}};
        if (snapshot.SingleMetricOptional(path, labels)) {
            return true;
        }
        engine::SleepFor(10ms);
    }
    return false;
}

}  // namespace

UTEST(OdbcStatementMetricsStorage, EnforcesLruBoundAndPromotesOnAccess) {
    static_assert(noexcept(std::declval<const detail::StatementStatsStorage&>()
                               .Account(std::string_view{}, 0, detail::StatementStatsStorage::ExecutionResult::kSuccess)
    ));

    detail::StatementStatsStorage storage{{.max_statements = 2}};

    storage.Account("first", 1, detail::StatementStatsStorage::ExecutionResult::kSuccess);
    storage.Account("second", 2, detail::StatementStatsStorage::ExecutionResult::kSuccess);
    storage.WaitForExhaustion();

    // Any event for an existing name promotes that name in the LRU.
    storage.Account("first", 3, detail::StatementStatsStorage::ExecutionResult::kError);
    storage.Account("third", 4, detail::StatementStatsStorage::ExecutionResult::kSuccess);
    storage.WaitForExhaustion();

    const auto snapshot = storage.GetStatementsStats();
    ASSERT_EQ(snapshot.statements.size(), 2);
    EXPECT_TRUE(snapshot.statements.contains("first"));
    EXPECT_FALSE(snapshot.statements.contains("second"));
    EXPECT_TRUE(snapshot.statements.contains("third"));
    EXPECT_EQ(snapshot.statements.at("first").executed.Load(), 1);
    EXPECT_EQ(snapshot.statements.at("first").errors.Load(), 1);
    EXPECT_EQ(snapshot.statements.at("first").timings.GetView().GetTotalCount(), 1);
}

UTEST(OdbcStatementMetricsStorage, ResizesDisablesClearsAndReenables) {
    detail::StatementStatsStorage storage{{.max_statements = 3}};
    for (const auto name : {"first", "second", "third"}) {
        storage.Account(name, 1, detail::StatementStatsStorage::ExecutionResult::kSuccess);
    }
    storage.WaitForExhaustion();

    storage.SetSettings({.max_statements = 2});
    auto snapshot = storage.GetStatementsStats();
    ASSERT_EQ(snapshot.statements.size(), 2);
    EXPECT_FALSE(snapshot.statements.contains("first"));
    EXPECT_TRUE(snapshot.statements.contains("second"));
    EXPECT_TRUE(snapshot.statements.contains("third"));

    // Flood beyond the bounded queue, then change generation without draining.
    // Already processed old events are cleared and queued old events are stale.
    for (std::size_t i = 0; i < 5000; ++i) {
        storage.Account("old-generation", 1, detail::StatementStatsStorage::ExecutionResult::kSuccess);
    }
    storage.SetSettings({.max_statements = 0});
    EXPECT_FALSE(storage.IsEnabled());
    EXPECT_TRUE(storage.GetStatementsStats().statements.empty());
    storage.Account("disabled", 1, detail::StatementStatsStorage::ExecutionResult::kSuccess);
    storage.SetSettings({.max_statements = 1});
    storage.WaitForExhaustion();
    EXPECT_TRUE(storage.IsEnabled());
    EXPECT_TRUE(storage.GetStatementsStats().statements.empty());
    storage.Account("reenabled", 1, detail::StatementStatsStorage::ExecutionResult::kSuccess);
    storage.WaitForExhaustion();

    snapshot = storage.GetStatementsStats();
    ASSERT_EQ(snapshot.statements.size(), 1);
    EXPECT_TRUE(snapshot.statements.contains("reenabled"));
    EXPECT_FALSE(snapshot.statements.contains("disabled"));
}

UTEST(OdbcStatementMetricsStorage, RejectsOperationGenerationAcrossDisableAndReenable) {
    detail::StatementStatsStorage storage{{.max_statements = 1}};
    const auto old_generation = storage.GetGenerationToken();
    ASSERT_TRUE(old_generation);

    storage.SetSettings({.max_statements = 0});
    storage.SetSettings({.max_statements = 1});
    storage.Account("stale-operation", 1, detail::StatementStatsStorage::ExecutionResult::kSuccess, *old_generation);
    storage.WaitForExhaustion();
    EXPECT_TRUE(storage.GetStatementsStats().statements.empty());

    storage.Account("fresh-operation", 1, detail::StatementStatsStorage::ExecutionResult::kSuccess);
    storage.WaitForExhaustion();
    const auto snapshot = storage.GetStatementsStats();
    ASSERT_EQ(snapshot.statements.size(), 1);
    EXPECT_TRUE(snapshot.statements.contains("fresh-operation"));

    detail::StatementStatsStorage disabled_storage{{.max_statements = 0}};
    const Query disabled_query{"SELECT 1", Query::Name{"started-disabled"}};
    detail::StatementStats disabled_tracker{disabled_query, disabled_storage};
    disabled_storage.SetSettings({.max_statements = 1});
    disabled_tracker.AccountSuccess();
    disabled_storage.WaitForExhaustion();
    EXPECT_TRUE(disabled_storage.GetStatementsStats().statements.empty());
}

UTEST(OdbcMetrics, ConnectionsBasic) {
    storages::odbc::Cluster cluster(kSettings, nullptr);

    utils::statistics::Storage statistics_storage;
    auto entry = statistics_storage.RegisterWriter("odbc", [&cluster](utils::statistics::Writer& writer) {
        cluster.WriteStatistics(writer);
    });

    UEXPECT_NO_THROW(cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1"));

    const utils::statistics::Snapshot snapshot{
        statistics_storage,
        "odbc",
        {{"odbc_pool", "0"}},
    };

    // Connection metrics
    EXPECT_GE(snapshot.SingleMetric("connections.opened").AsRate(), 1);
    EXPECT_EQ(snapshot.SingleMetric("connections.closed").AsRate(), 0);
    EXPECT_GE(snapshot.SingleMetric("connections.active").AsInt(), 1);

    // Query metrics
    EXPECT_GE(snapshot.SingleMetric("queries.executed").AsRate(), 1);

    // Transaction metrics (out-of-transaction query)
    EXPECT_GE(snapshot.SingleMetric("transactions.no-tran").AsRate(), 1);

    entry.Unregister();
}

UTEST(OdbcMetrics, TransactionMetrics) {
    storages::odbc::Cluster cluster(kSettings, nullptr);

    utils::statistics::Storage statistics_storage;
    auto entry = statistics_storage.RegisterWriter("odbc", [&cluster](utils::statistics::Writer& writer) {
        cluster.WriteStatistics(writer);
    });

    {
        auto tx = cluster.Begin(storages::odbc::ClusterHostType::kMaster);
        UEXPECT_NO_THROW(tx.Execute("SELECT 1"));
        UEXPECT_NO_THROW(tx.Execute("SELECT 2"));
        tx.Commit();
    }

    const utils::statistics::Snapshot snapshot{
        statistics_storage,
        "odbc",
        {{"odbc_pool", "0"}},
    };

    // Transaction metrics
    EXPECT_GE(snapshot.SingleMetric("transactions.total").AsRate(), 1);
    EXPECT_GE(snapshot.SingleMetric("transactions.committed").AsRate(), 1);
    EXPECT_EQ(snapshot.SingleMetric("transactions.rolled-back").AsRate(), 0);

    // Query metrics (2 queries in transaction)
    EXPECT_GE(snapshot.SingleMetric("queries.executed").AsRate(), 2);

    entry.Unregister();
}

UTEST(OdbcMetrics, RollbackMetrics) {
    storages::odbc::Cluster cluster(kSettings, nullptr);

    utils::statistics::Storage statistics_storage;
    auto entry = statistics_storage.RegisterWriter("odbc", [&cluster](utils::statistics::Writer& writer) {
        cluster.WriteStatistics(writer);
    });

    {
        auto tx = cluster.Begin(storages::odbc::ClusterHostType::kMaster);
        UEXPECT_NO_THROW(tx.Execute("SELECT 1"));
        tx.Rollback();
    }

    const utils::statistics::Snapshot snapshot{
        statistics_storage,
        "odbc",
        {{"odbc_pool", "0"}},
    };

    EXPECT_GE(snapshot.SingleMetric("transactions.total").AsRate(), 1);
    EXPECT_EQ(snapshot.SingleMetric("transactions.committed").AsRate(), 0);
    EXPECT_GE(snapshot.SingleMetric("transactions.rolled-back").AsRate(), 1);

    entry.Unregister();
}

UTEST(OdbcMetrics, ErrorMetrics) {
    storages::odbc::Cluster cluster(kSettings, nullptr);

    utils::statistics::Storage statistics_storage;
    auto entry = statistics_storage.RegisterWriter("odbc", [&cluster](utils::statistics::Writer& writer) {
        cluster.WriteStatistics(writer);
    });

    // Execute a valid query first to ensure connection is established
    UEXPECT_NO_THROW(cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1"));

    // Try to execute an invalid query
    UEXPECT_THROW(
        cluster.Execute(storages::odbc::ClusterHostType::kMaster, "INVALID SQL SYNTAX"),
        storages::odbc::Error
    );

    const utils::statistics::Snapshot snapshot{
        statistics_storage,
        "odbc",
        {{"odbc_pool", "0"}},
    };

    EXPECT_GE(snapshot.SingleMetric("errors", {{"odbc_error", "query-exec"}}).AsRate(), 1);

    entry.Unregister();
}

UTEST(OdbcStatementMetrics, DisabledAndUnnamedQueriesDoNotCreateSeries) {
    storages::odbc::Cluster cluster(kSettings, nullptr);
    utils::statistics::Storage statistics_storage;
    auto entry = statistics_storage.RegisterWriter("odbc", [&cluster](utils::statistics::Writer& writer) {
        cluster.WriteStatistics(writer);
    });

    cluster.Execute(ClusterHostType::kMaster, Query{"SELECT 1", Query::Name{"disabled-named-query"}});
    cluster.SetStatementMetricsSettings({.max_statements = 10});
    cluster.Execute(ClusterHostType::kMaster, "SELECT 1");
    engine::SleepFor(20ms);

    const utils::statistics::Snapshot snapshot{
        statistics_storage,
        "odbc",
        {{"odbc_pool", "0"}},
    };
    EXPECT_FALSE(snapshot.SingleMetricOptional("statement_executed", {{"odbc_query", "disabled-named-query"}}));
    EXPECT_FALSE(snapshot.SingleMetricOptional("statement_executed"));

    entry.Unregister();
}

UTEST(OdbcStatementMetrics, AccountsSuccessErrorsTimeoutsTransactionsAndParameterStore) {
    storages::odbc::Cluster cluster(kSettings, nullptr);

    utils::statistics::Storage statistics_storage;
    auto entry = statistics_storage.RegisterWriter("odbc", [&cluster](utils::statistics::Writer& writer) {
        cluster.WriteStatistics(writer);
    });

    /// [ODBC named query metrics]
    cluster.SetStatementMetricsSettings({.max_statements = 10});
    const Query success{"SELECT 1", Query::Name{"named-success"}};
    cluster.Execute(ClusterHostType::kMaster, success);
    /// [ODBC named query metrics]
    cluster.Execute(ClusterHostType::kMaster, Query{"SELECT 1", Query::Name{""}});

    ParameterStore parameters;
    parameters.PushBack(42);
    cluster.Execute(
        ClusterHostType::kMaster,
        Query{"SELECT ?::integer", Query::Name{"named-parameter-store"}},
        parameters
    );

    UEXPECT_THROW(
        cluster.Execute(ClusterHostType::kMaster, Query{"INVALID SQL SYNTAX", Query::Name{"named-syntax-error"}}),
        Error
    );

    UEXPECT_THROW(
        cluster.Execute(
            ClusterHostType::kMaster,
            CommandControl{.statement_timeout = 1ms},
            Query{"SELECT pg_sleep(CAST(? AS double precision))", Query::Name{"named-timeout"}},
            0.05
        ),
        OperationInterrupted
    );

    UEXPECT_THROW(
        cluster.Execute(
            ClusterHostType::kMaster,
            CommandControl{.network_timeout = 1s, .statement_timeout = 0ms},
            Query{"SELECT 1", Query::Name{"named-direct-preflight-timeout"}}
        ),
        OperationInterrupted
    );

    auto transaction = cluster.Begin(ClusterHostType::kMaster);
    transaction.Execute(Query{"SELECT 1", Query::Name{"named-transaction"}});
    transaction.Execute(Query{"SELECT ?::integer", Query::Name{"named-transaction-parameter-store"}}, parameters);
    transaction.Commit();

    auto preflight_transaction = cluster.Begin(ClusterHostType::kMaster);
    UEXPECT_THROW(
        preflight_transaction.Execute(
            CommandControl{.network_timeout = 1s, .statement_timeout = 0ms},
            Query{"SELECT 1", Query::Name{"named-transaction-preflight-timeout"}}
        ),
        OperationInterrupted
    );
    preflight_transaction.Rollback();

    for (const auto name : {
             "named-success",
             "named-parameter-store",
             "named-transaction",
             "named-transaction-parameter-store",
         })
    {
        ASSERT_TRUE(WaitForMetric(statistics_storage, "statement_executed", {{"odbc_query", name}})) << name;
        ASSERT_TRUE(WaitForMetric(statistics_storage, "statement_timings", {{"odbc_query", name}})) << name;
        const utils::statistics::Snapshot snapshot{
            statistics_storage,
            "odbc",
            {{"odbc_pool", "0"}},
        };
        EXPECT_EQ(snapshot.SingleMetric("statement_executed", {{"odbc_query", name}}).AsRate(), 1) << name;
        EXPECT_EQ(snapshot.SingleMetric("statement_timings", {{"odbc_query", name}}).AsHistogram().GetTotalCount(), 1)
            << name;
    }

    ASSERT_TRUE(WaitForMetric(statistics_storage, "statement_executed", {{"odbc_query", ""}}));
    const utils::statistics::Snapshot empty_name_snapshot{
        statistics_storage,
        "odbc",
        {{"odbc_pool", "0"}},
    };
    EXPECT_EQ(empty_name_snapshot.SingleMetric("statement_executed", {{"odbc_query", ""}}).AsRate(), 1);

    for (const auto name : {
             "named-syntax-error",
             "named-timeout",
             "named-direct-preflight-timeout",
             "named-transaction-preflight-timeout",
         })
    {
        ASSERT_TRUE(WaitForMetric(statistics_storage, "statement_errors", {{"odbc_query", name}})) << name;

        const utils::statistics::Snapshot snapshot{
            statistics_storage,
            "odbc",
            {{"odbc_pool", "0"}},
        };
        EXPECT_EQ(snapshot.SingleMetric("statement_errors", {{"odbc_query", name}}).AsRate(), 1) << name;
        EXPECT_EQ(snapshot.SingleMetric("statement_executed", {{"odbc_query", name}}).AsRate(), 0);
        EXPECT_EQ(snapshot.SingleMetric("statement_timings", {{"odbc_query", name}}).AsHistogram().GetTotalCount(), 0);
    }

    const utils::statistics::Snapshot aggregate_snapshot{
        statistics_storage,
        "odbc",
        {{"odbc_pool", "0"}},
    };
    EXPECT_EQ(aggregate_snapshot.SingleMetric("errors", {{"odbc_error", "query-timeout"}}).AsRate(), 3);

    entry.Unregister();
}

UTEST(OdbcStatementMetrics, TopologyReloadUsesCurrentSettingAndOldTransactionRemainsSafe) {
    storages::odbc::Cluster cluster(kSettings, nullptr);
    cluster.SetStatementMetricsSettings({.max_statements = 1});
    auto old_transaction = cluster.Begin(ClusterHostType::kMaster);

    auto updated_settings = kSettings;
    updated_settings.pools.front().pool.max_size += 1;
    cluster.UpdateSettings(updated_settings);

    utils::statistics::Storage statistics_storage;
    auto entry = statistics_storage.RegisterWriter("odbc", [&cluster](utils::statistics::Writer& writer) {
        cluster.WriteStatistics(writer);
    });

    old_transaction.Execute(Query{"SELECT 1", Query::Name{"old-topology-transaction"}});
    old_transaction.Commit();
    cluster.Execute(ClusterHostType::kMaster, Query{"SELECT 1", Query::Name{"new-topology-first"}});
    cluster.Execute(ClusterHostType::kMaster, Query{"SELECT 1", Query::Name{"new-topology-second"}});

    ASSERT_TRUE(WaitForMetric(statistics_storage, "statement_executed", {{"odbc_query", "new-topology-second"}}));

    // The writer follows the newly published topology, while the old transaction
    // safely retained and used its original pool.
    const utils::statistics::Snapshot snapshot{
        statistics_storage,
        "odbc",
        {{"odbc_pool", "0"}},
    };
    EXPECT_EQ(snapshot.SingleMetric("statement_executed", {{"odbc_query", "new-topology-second"}}).AsRate(), 1);
    EXPECT_FALSE(snapshot.SingleMetricOptional("statement_executed", {{"odbc_query", "old-topology-transaction"}}));
    EXPECT_FALSE(snapshot.SingleMetricOptional("statement_executed", {{"odbc_query", "new-topology-first"}}));

    entry.Unregister();
}

}  // namespace storages::odbc::tests

USERVER_NAMESPACE_END
