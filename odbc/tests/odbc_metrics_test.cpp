#include <userver/utest/utest.hpp>

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <storages/odbc/detail/connection.hpp>
#include <storages/odbc/detail/prepared_statement_cache.hpp>
#include <storages/odbc/detail/statement_stats.hpp>
#include <storages/odbc/detail/statement_stats_storage.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/current_task.hpp>
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

UTEST(OdbcPreparedStatementCacheState, DisableReenableAdvancesResetGeneration) {
    detail::PreparedStatementCacheState state{{.max_size = 2}};
    const auto initial = state.GetSettings();
    EXPECT_EQ(initial.max_size, 2);

    state.SetSettings({.max_size = 3});
    const auto grown = state.GetSettings();
    EXPECT_EQ(grown.max_size, 3);
    EXPECT_EQ(grown.reset_generation, initial.reset_generation);

    state.SetSettings({.max_size = 0});
    state.SetSettings({.max_size = 3});
    const auto reenabled = state.GetSettings();
    EXPECT_EQ(reenabled.max_size, 3);
    EXPECT_EQ(reenabled.reset_generation, initial.reset_generation + 2);
}

UTEST(OdbcPreparedStatementCache, DirectTransactionParameterStoreAndExactSqlKey) {
    const auto host = settings::HostSettings{kDSN, {.min_size = 1, .max_size = 1}};
    Cluster cluster{settings::ODBCClusterSettings{{host}}, nullptr};

    utils::statistics::Storage statistics_storage;
    auto entry = statistics_storage.RegisterWriter("odbc", [&cluster](utils::statistics::Writer& writer) {
        cluster.WriteStatistics(writer);
    });

    const utils::statistics::Snapshot before_zero_parameter{
        statistics_storage,
        "odbc",
        {{"odbc_pool", "0"}},
    };
    const auto misses_before_zero = before_zero_parameter.SingleMetric("queries.prepared-cache-misses").AsRate();
    const auto hits_before_zero = before_zero_parameter.SingleMetric("queries.prepared-cache-hits").AsRate();
    const auto current_before_zero = before_zero_parameter.SingleMetric("connections.prepared-statements").AsInt();

    cluster.Execute(ClusterHostType::kMaster, "SELECT 1");
    cluster.Execute(ClusterHostType::kMaster, "SELECT 1");
    {
        const utils::statistics::Snapshot zero_parameter_snapshot{
            statistics_storage,
            "odbc",
            {{"odbc_pool", "0"}},
        };
        EXPECT_EQ(zero_parameter_snapshot.SingleMetric("queries.prepared-cache-misses").AsRate(), misses_before_zero);
        EXPECT_EQ(zero_parameter_snapshot.SingleMetric("queries.prepared-cache-hits").AsRate(), hits_before_zero);
        EXPECT_EQ(zero_parameter_snapshot.SingleMetric("connections.prepared-statements").AsInt(), current_before_zero);
    }

    /// [ODBC prepared statement cache]
    cluster.SetPreparedStatementCacheSettings({.max_size = 3});
    const Query same_sql_first_name{"SELECT ?::text", Query::Name{"first-name"}};
    const Query same_sql_second_name{"SELECT ?::text", Query::Name{"second-name"}};
    EXPECT_EQ(cluster.Execute(ClusterHostType::kMaster, same_sql_first_name, "first")[0][0].GetString(), "first");
    EXPECT_EQ(
        cluster.Execute(ClusterHostType::kMaster, same_sql_second_name, "a much longer value")[0][0].GetString(),
        "a much longer value"
    );
    /// [ODBC prepared statement cache]

    ParameterStore parameters;
    parameters.PushBack(std::string{"from-store"});
    EXPECT_EQ(
        cluster.Execute(ClusterHostType::kMaster, same_sql_first_name, parameters)[0][0].GetString(),
        "from-store"
    );

    auto transaction = cluster.Begin(ClusterHostType::kMaster);
    EXPECT_EQ(transaction.Execute(same_sql_second_name, "in-transaction")[0][0].GetString(), "in-transaction");
    EXPECT_EQ(
        transaction.Execute(Query{"SELECT ?::text || '-other'", Query::Name{"first-name"}}, "sql")[0][0].GetString(),
        "sql-other"
    );
    transaction.Commit();

    // Reuse the exact SQL across materially different compatible bindings.
    const Query numeric_query{"SELECT ?::numeric"};
    const Decimal<9, 4> decimal{"12.3400"};
    EXPECT_EQ(cluster.Execute(ClusterHostType::kMaster, numeric_query, decimal)[0][0].GetString(), "12.3400");
    EXPECT_EQ(cluster.Execute(ClusterHostType::kMaster, numeric_query, std::int64_t{42})[0][0].GetInt64(), 42);
    EXPECT_DOUBLE_EQ(cluster.Execute(ClusterHostType::kMaster, numeric_query, 1.25)[0][0].GetDouble(), 1.25);

    const utils::statistics::Snapshot snapshot{statistics_storage, "odbc", {{"odbc_pool", "0"}}};
    EXPECT_EQ(snapshot.SingleMetric("queries.prepared-cache-misses").AsRate(), 3);
    EXPECT_EQ(snapshot.SingleMetric("queries.prepared-cache-hits").AsRate(), 5);
    EXPECT_EQ(snapshot.SingleMetric("queries.prepared-cache-evictions").AsRate(), 0);
    EXPECT_EQ(snapshot.SingleMetric("connections.prepared-statements").AsInt(), 3);

    entry.Unregister();
}

UTEST(OdbcPreparedStatementCache, DisabledByDefaultAndCachedTimeoutIsResetToUnlimited) {
    {
        Cluster disabled_cluster{kSettings, nullptr};
        utils::statistics::Storage statistics_storage;
        auto entry = statistics_storage.RegisterWriter("odbc", [&disabled_cluster](utils::statistics::Writer& writer) {
            disabled_cluster.WriteStatistics(writer);
        });
        disabled_cluster.Execute(ClusterHostType::kMaster, "SELECT ?::integer", 1);
        disabled_cluster.Execute(ClusterHostType::kMaster, "SELECT ?::integer", 2);
        const utils::statistics::Snapshot snapshot{statistics_storage, "odbc", {{"odbc_pool", "0"}}};
        EXPECT_EQ(snapshot.SingleMetric("queries.prepared-cache-misses").AsRate(), 0);
        EXPECT_EQ(snapshot.SingleMetric("queries.prepared-cache-hits").AsRate(), 0);
        EXPECT_EQ(snapshot.SingleMetric("connections.prepared-statements").AsInt(), 0);
        entry.Unregister();
    }

    auto cache_state = std::make_shared<detail::PreparedStatementCacheState>(settings::PreparedStatementCacheSettings{
        .max_size = 1
    });
    {
        Connection connection{
            kDSN,
            engine::current_task::GetBlockingTaskProcessor(),
            engine::Deadline::FromDuration(2s),
            cache_state,
        };
        const Query query{"SELECT ?::integer FROM pg_sleep(?::double precision)"};
        connection.Query(query, impl::MakeParameterList(1, 0.0), engine::Deadline::FromDuration(100ms));

        // The first execution writes a one-second ODBC timeout. The cache hit
        // must explicitly overwrite it with 0 for an unreachable deadline.
        const auto result = connection.Query(query, impl::MakeParameterList(2, 1.1), engine::Deadline{});
        ASSERT_EQ(result.Size(), 1);
        EXPECT_EQ(result[0][0].GetInt32(), 2);
        EXPECT_EQ(cache_state->GetStatistics().misses.Load().value, 1);
        EXPECT_EQ(cache_state->GetStatistics().hits.Load().value, 1);
        EXPECT_EQ(cache_state->GetStatistics().current.Load(), 1);
    }
    EXPECT_EQ(cache_state->GetStatistics().current.Load(), 0);
}

UTEST(OdbcPreparedStatementCache, DynamicResetIsAppliedAtTransactionOperations) {
    const auto host = settings::HostSettings{kDSN, {.min_size = 1, .max_size = 1}};
    Cluster cluster{settings::ODBCClusterSettings{{host}}, nullptr};
    cluster.SetPreparedStatementCacheSettings({.max_size = 1});

    utils::statistics::Storage statistics_storage;
    auto entry = statistics_storage.RegisterWriter("odbc", [&cluster](utils::statistics::Writer& writer) {
        cluster.WriteStatistics(writer);
    });
    const auto current = [&] {
        return utils::statistics::Snapshot{statistics_storage, "odbc", {{"odbc_pool", "0"}}}
            .SingleMetric("connections.prepared-statements")
            .AsInt();
    };
    const auto evictions = [&] {
        return utils::statistics::Snapshot{statistics_storage, "odbc", {{"odbc_pool", "0"}}}
            .SingleMetric("queries.prepared-cache-evictions")
            .AsRate();
    };

    auto commit_transaction = cluster.Begin(ClusterHostType::kMaster);
    commit_transaction.Execute("SELECT ?::integer", 1);
    ASSERT_EQ(current(), 1);
    cluster.SetPreparedStatementCacheSettings({.max_size = 0});
    commit_transaction.Commit();
    EXPECT_EQ(current(), 0);
    EXPECT_EQ(evictions(), 0);

    cluster.SetPreparedStatementCacheSettings({.max_size = 1});
    cluster.Execute(ClusterHostType::kMaster, "SELECT ?::integer", 2);
    ASSERT_EQ(current(), 1);
    cluster.SetPreparedStatementCacheSettings({.max_size = 0});
    auto begin_transaction = cluster.Begin(ClusterHostType::kMaster);
    EXPECT_EQ(current(), 0);
    begin_transaction.Rollback();
    EXPECT_EQ(evictions(), 0);

    cluster.SetPreparedStatementCacheSettings({.max_size = 1});
    auto rollback_transaction = cluster.Begin(ClusterHostType::kMaster);
    rollback_transaction.Execute("SELECT ?::integer", 3);
    ASSERT_EQ(current(), 1);
    cluster.SetPreparedStatementCacheSettings({.max_size = 0});
    rollback_transaction.Rollback();
    EXPECT_EQ(current(), 0);
    EXPECT_EQ(evictions(), 0);

    entry.Unregister();
}

UTEST(OdbcPreparedStatementCache, LruResizeDisableErrorsAndReenable) {
    const auto host = settings::HostSettings{kDSN, {.min_size = 1, .max_size = 1}};
    Cluster cluster{settings::ODBCClusterSettings{{host}}, nullptr};
    cluster.SetPreparedStatementCacheSettings({.max_size = 2});

    utils::statistics::Storage statistics_storage;
    auto entry = statistics_storage.RegisterWriter("odbc", [&cluster](utils::statistics::Writer& writer) {
        cluster.WriteStatistics(writer);
    });

    cluster.Execute(ClusterHostType::kMaster, "SELECT ?::integer", 1);
    cluster.Execute(ClusterHostType::kMaster, "SELECT ?::bigint", 2);
    cluster.Execute(ClusterHostType::kMaster, "SELECT ?::integer", 3);  // promote
    cluster.Execute(ClusterHostType::kMaster, "SELECT ?::text", "third");
    cluster.Execute(ClusterHostType::kMaster, "SELECT ?::bigint", 4);  // evicted LRU => miss

    // A hit that fails before SQLExecute is evicted and never retried.
    UEXPECT_THROW(cluster.Execute(ClusterHostType::kMaster, "SELECT ?::bigint", 1, 2), StatementError);
    cluster.Execute(ClusterHostType::kMaster, "SELECT ?::bigint", 5);

    // An execution error also evicts the affected cached handle.
    cluster.Execute(ClusterHostType::kMaster, "SELECT 100 / ?::integer", 2);
    UEXPECT_THROW(cluster.Execute(ClusterHostType::kMaster, "SELECT 100 / ?::integer", 0), StatementError);
    cluster.Execute(ClusterHostType::kMaster, "SELECT 100 / ?::integer", 4);

    cluster.SetPreparedStatementCacheSettings({.max_size = 1});
    cluster.Execute(ClusterHostType::kMaster, "SELECT ?::integer", 6);
    auto snapshot = utils::statistics::Snapshot{statistics_storage, "odbc", {{"odbc_pool", "0"}}};
    EXPECT_EQ(snapshot.SingleMetric("connections.prepared-statements").AsInt(), 1);
    const auto evictions_before_disable = snapshot.SingleMetric("queries.prepared-cache-evictions").AsRate();

    cluster.SetPreparedStatementCacheSettings({.max_size = 0});
    cluster.Execute(ClusterHostType::kMaster, "SELECT ?::integer", 7);
    const utils::statistics::Snapshot disabled_snapshot{statistics_storage, "odbc", {{"odbc_pool", "0"}}};
    EXPECT_EQ(disabled_snapshot.SingleMetric("connections.prepared-statements").AsInt(), 0);
    EXPECT_EQ(disabled_snapshot.SingleMetric("queries.prepared-cache-evictions").AsRate(), evictions_before_disable);

    // Skipping the disabled generation still forces an empty cache on reenable.
    cluster.SetPreparedStatementCacheSettings({.max_size = 1});
    cluster.Execute(ClusterHostType::kMaster, "SELECT ?::integer", 8);
    const utils::statistics::Snapshot reenabled_snapshot{statistics_storage, "odbc", {{"odbc_pool", "0"}}};
    EXPECT_EQ(reenabled_snapshot.SingleMetric("connections.prepared-statements").AsInt(), 1);

    entry.Unregister();
}

UTEST(OdbcPreparedStatementCache, PerPhysicalConnectionBoundAndTopologyReload) {
    const auto host = settings::HostSettings{kDSN, {.min_size = 2, .max_size = 2}};
    const settings::ODBCClusterSettings initial{{host}};
    Cluster cluster{initial, nullptr};
    cluster.SetPreparedStatementCacheSettings({.max_size = 1});

    utils::statistics::Storage statistics_storage;
    auto entry = statistics_storage.RegisterWriter("odbc", [&cluster](utils::statistics::Writer& writer) {
        cluster.WriteStatistics(writer);
    });

    auto first = cluster.Begin(ClusterHostType::kMaster);
    auto second = cluster.Begin(ClusterHostType::kMaster);
    first.Execute("SELECT ?::integer", 1);
    first.Execute("SELECT ?::bigint", 2);
    second.Execute("SELECT ?::text", "second");
    second.Execute("SELECT ?::boolean", true);

    auto snapshot = utils::statistics::Snapshot{statistics_storage, "odbc", {{"odbc_pool", "0"}}};
    EXPECT_EQ(snapshot.SingleMetric("connections.prepared-statements").AsInt(), 2);
    EXPECT_LE(
        snapshot.SingleMetric("connections.prepared-statements").AsInt(),
        snapshot.SingleMetric("connections.active").AsInt()
    );

    auto reloaded = initial;
    reloaded.pools.front().pool.min_size = 1;
    reloaded.pools.front().pool.max_size = 3;
    cluster.UpdateSettings(reloaded);
    cluster.Execute(ClusterHostType::kMaster, "SELECT ?::integer", 10);
    cluster.Execute(ClusterHostType::kMaster, "SELECT ?::integer", 11);

    first.Commit();
    second.Rollback();
    const utils::statistics::Snapshot reloaded_snapshot{statistics_storage, "odbc", {{"odbc_pool", "0"}}};
    EXPECT_EQ(reloaded_snapshot.SingleMetric("queries.prepared-cache-misses").AsRate(), 1);
    EXPECT_EQ(reloaded_snapshot.SingleMetric("queries.prepared-cache-hits").AsRate(), 1);
    EXPECT_EQ(reloaded_snapshot.SingleMetric("connections.prepared-statements").AsInt(), 1);

    entry.Unregister();
}

}  // namespace storages::odbc::tests

USERVER_NAMESPACE_END
