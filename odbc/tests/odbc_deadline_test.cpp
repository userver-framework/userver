#include <storages/odbc/detail/command_control_store.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/storages/odbc.hpp>
#include <userver/storages/odbc/exception.hpp>
#include <userver/storages/odbc/tests/utils.hpp>
#include <userver/utest/utest.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::tests {

namespace {

server::request::TaskInheritedData MakeRequestData(
    engine::Deadline deadline,
    std::string_view path = {},
    std::string_view method = "GET"
) {
    return {
        .path = path,
        .method = method,
        .start_time = {},
        .deadline = std::move(deadline),
    };
}

}  // namespace

UTEST(OdbcCommandControl, ResolvesEveryFieldByLayerAndSupportsTransparentLookup) {
    detail::CommandControlConfig config{
        .default_command_control = {.network_timeout = 10ms, .statement_timeout = 20ms},
        .handlers_command_control = {{
            "/handler",
            CommandControlByMethodMap{{"GET", {.network_timeout = 30ms}}},
        }},
        .queries_command_control = {{"named", {.network_timeout = 40ms, .statement_timeout = 40ms}}},
    };

    const std::string_view handler_path = "/handler";
    const std::string_view method = "GET";
    const std::string_view query_name = "named";
    EXPECT_NE(config.handlers_command_control.find(handler_path), config.handlers_command_control.end());
    EXPECT_NE(config.queries_command_control.find(query_name), config.queries_command_control.end());

    const auto resolved = detail::ResolveCommandControl(
        config,
        handler_path,
        method,
        Query::NameView{"named"},
        CommandControl{.network_timeout = 50ms}
    );
    EXPECT_EQ(resolved.network_timeout, 50ms);
    EXPECT_EQ(resolved.statement_timeout, 40ms);

    const auto query_over_handler =
        detail::ResolveCommandControl(config, handler_path, method, Query::NameView{"named"}, std::nullopt);
    EXPECT_EQ(query_over_handler.network_timeout, 40ms);
    EXPECT_EQ(query_over_handler.statement_timeout, 40ms);

    const auto unnamed = detail::ResolveCommandControl(config, handler_path, method, std::nullopt, std::nullopt);
    EXPECT_EQ(unnamed.network_timeout, 30ms);
    EXPECT_EQ(unnamed.statement_timeout, 20ms);
    EXPECT_EQ((CommandControl{.network_timeout = 1ms}), (CommandControl{.network_timeout = 1ms}));
}

UTEST(OdbcCommandControl, ReplacesAndResetsMapsAndRefreshesTransactionQueries) {
    detail::CommandControlStore store;
    store.Assign({
        .default_command_control = {.network_timeout = 100ms, .statement_timeout = 200ms},
        .handlers_command_control = {},
        .queries_command_control = {{"named", {.network_timeout = 10ms}}},
    });
    const Query query{"SELECT 1", Query::Name{"named"}};
    const CommandControl transaction_base{.network_timeout = 100ms, .statement_timeout = 200ms};

    auto resolved = store.ResolveTransactionStatement(transaction_base, query.GetOptionalNameView(), std::nullopt);
    EXPECT_EQ(resolved.network_timeout, 10ms);
    EXPECT_EQ(resolved.statement_timeout, 200ms);

    store.SetQueries({{"named", {.statement_timeout = 20ms}}});
    resolved = store.ResolveTransactionStatement(transaction_base, query.GetOptionalNameView(), std::nullopt);
    EXPECT_EQ(resolved.network_timeout, 100ms);
    EXPECT_EQ(resolved.statement_timeout, 20ms);

    store.SetQueries({});
    resolved = store.ResolveTransactionStatement(
        transaction_base,
        query.GetOptionalNameView(),
        CommandControl{.network_timeout = 30ms}
    );
    EXPECT_EQ(resolved.network_timeout, 30ms);
    EXPECT_EQ(resolved.statement_timeout, 200ms);

    store.Assign({});
    const auto reset = store.ReadCopy();
    EXPECT_EQ(reset.default_command_control, CommandControl{});
    EXPECT_TRUE(reset.handlers_command_control.empty());
    EXPECT_TRUE(reset.queries_command_control.empty());
    EXPECT_EQ(store.Resolve("/handler", "GET", query.GetOptionalNameView(), std::nullopt), CommandControl{});
}

UTEST(OdbcDeadline, CancelledByInheritedDeadlineOnDefaultExecute) {
    auto cluster = MakeCluster();
    server::request::kTaskInheritedData.Set(MakeRequestData(engine::Deadline::FromDuration(-1s)));

    UEXPECT_THROW(
        cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1"),
        storages::odbc::OperationInterrupted
    );
}

UTEST(OdbcDeadline, InheritedExpiredOverridesLongExplicitExecute) {
    auto cluster = MakeCluster();
    server::request::kTaskInheritedData.Set(MakeRequestData(engine::Deadline::Passed()));

    UEXPECT_THROW(
        cluster.Execute(
            storages::odbc::ClusterHostType::kMaster,
            storages::odbc::CommandControl{.statement_timeout = 1h},
            "SELECT 1"
        ),
        storages::odbc::OperationInterrupted
    );
}

UTEST(OdbcDeadline, ExplicitExpiredExecute) {
    auto cluster = MakeCluster();

    UEXPECT_THROW(
        cluster.Execute(
            storages::odbc::ClusterHostType::kMaster,
            storages::odbc::CommandControl{.statement_timeout = 0ms},
            "SELECT 1"
        ),
        storages::odbc::OperationInterrupted
    );
}

UTEST(OdbcDeadline, ExpiresDuringBlockingExecute) {
    auto cluster = MakeCluster();

    UEXPECT_THROW(
        cluster.Execute(
            storages::odbc::ClusterHostType::kMaster,
            storages::odbc::CommandControl{.statement_timeout = 1ms},
            "SELECT pg_sleep(CAST(? AS double precision))",
            0.05
        ),
        storages::odbc::OperationInterrupted
    );
}

UTEST(OdbcDeadline, CancelledByInheritedDeadlineOnBegin) {
    auto cluster = MakeCluster();
    server::request::kTaskInheritedData.Set(MakeRequestData(engine::Deadline::FromDuration(-1s)));

    UEXPECT_THROW(cluster.Begin(storages::odbc::ClusterHostType::kMaster), storages::odbc::OperationInterrupted);
}

UTEST(OdbcDeadline, DeadlinePropagationBlockerIgnoresInheritedExpiry) {
    auto cluster = MakeCluster();
    server::request::kTaskInheritedData.Set(MakeRequestData(engine::Deadline::Passed()));

    server::request::DeadlinePropagationBlocker blocker;
    UEXPECT_NO_THROW(cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1"));
}

UTEST(OdbcCommandControl, AppliesHandlerQueryExplicitAndParameterStoreLayers) {
    auto cluster = MakeCluster();
    cluster.SetDefaultCommandControl({.network_timeout = 1s, .statement_timeout = 1s});
    cluster.SetHandlersCommandControl({{
        "/command-control",
        CommandControlByMethodMap{{"GET", {.statement_timeout = 0ms}}},
    }});
    server::request::kTaskInheritedData.Set(MakeRequestData(engine::Deadline{}, "/command-control", "GET"));
    UEXPECT_THROW(cluster.Execute(ClusterHostType::kMaster, "SELECT 1"), OperationInterrupted);

    server::request::kTaskInheritedData.Set(MakeRequestData(engine::Deadline{}, "/command-control", "POST"));
    UEXPECT_NO_THROW(cluster.Execute(ClusterHostType::kMaster, "SELECT 1"));

    const Query named{"SELECT 1", Query::Name{"named-command-control"}};
    cluster.SetQueriesCommandControl({{"named-command-control", {.network_timeout = 0ms}}});
    UEXPECT_NO_THROW(cluster.Execute(ClusterHostType::kMaster, "SELECT 1"));
    UEXPECT_THROW(cluster.Execute(ClusterHostType::kMaster, named), OperationInterrupted);
    UEXPECT_NO_THROW(cluster.Execute(ClusterHostType::kMaster, CommandControl{.network_timeout = 1s}, named));
    UEXPECT_THROW(
        cluster.Execute(ClusterHostType::kMaster, CommandControl{.statement_timeout = 1s}, named),
        OperationInterrupted
    );

    ParameterStore parameters;
    UEXPECT_THROW(cluster.Execute(ClusterHostType::kMaster, named, parameters), OperationInterrupted);
    UEXPECT_NO_THROW(cluster.Execute(ClusterHostType::kMaster, CommandControl{.network_timeout = 1s}, named, parameters)
    );
    server::request::kTaskInheritedData.Erase();
}

UTEST(OdbcCommandControl, NamedQueryConfigurationExample) {
    auto cluster = MakeCluster();

    /// [ODBC named query command control]
    cluster.SetDefaultCommandControl({.network_timeout = 3s, .statement_timeout = 3s});
    cluster.SetHandlersCommandControl({{
        "/v1/items/{id}",
        CommandControlByMethodMap{{"GET", {.network_timeout = 2s}}},
    }});
    cluster.SetQueriesCommandControl({{
        "select-item",
        {.statement_timeout = 1s},
    }});

    const Query query{
        "SELECT ?::integer",
        Query::Name{"select-item"},
    };
    const auto result = cluster.Execute(ClusterHostType::kMaster, CommandControl{.network_timeout = 1500ms}, query, 42);
    /// [ODBC named query command control]

    ASSERT_EQ(result.Size(), 1);
    EXPECT_EQ(result[0][0].GetInt32(), 42);
}

UTEST(OdbcCommandControl, TransactionCapturesBaseAndRefreshesNamedQueries) {
    auto cluster = MakeCluster();
    cluster.SetDefaultCommandControl({.network_timeout = 1s, .statement_timeout = 1s});
    cluster.SetHandlersCommandControl({{
        "/transaction-command-control",
        CommandControlByMethodMap{{"GET", {.statement_timeout = 2s}}},
    }});
    server::request::kTaskInheritedData.Set(MakeRequestData(engine::Deadline{}, "/transaction-command-control", "GET"));
    auto transaction = cluster.Begin(ClusterHostType::kMaster);

    cluster.SetHandlersCommandControl({{
        "/transaction-command-control",
        CommandControlByMethodMap{{"GET", {.statement_timeout = 0ms}}},
    }});
    UEXPECT_NO_THROW(transaction.Execute("SELECT 1"));

    const Query named{"SELECT 1", Query::Name{"transaction-fresh-query"}};
    cluster.SetQueriesCommandControl({{"transaction-fresh-query", {.network_timeout = 0ms}}});
    UEXPECT_THROW(transaction.Execute(named), OperationInterrupted);
    UEXPECT_NO_THROW(transaction.Execute(CommandControl{.network_timeout = 1s}, named));
    UEXPECT_THROW(transaction.Execute(CommandControl{.statement_timeout = 1s}, named), OperationInterrupted);

    ParameterStore parameters;
    UEXPECT_THROW(transaction.Execute(named, parameters), OperationInterrupted);
    UEXPECT_NO_THROW(transaction.Execute(CommandControl{.network_timeout = 1s}, named, parameters));
    cluster.SetQueriesCommandControl({{"transaction-fresh-query", {.network_timeout = 1s}}});
    UEXPECT_NO_THROW(transaction.Execute(named));
    cluster.SetQueriesCommandControl({});
    UEXPECT_NO_THROW(transaction.Execute(named));
    transaction.Commit();
    server::request::kTaskInheritedData.Erase();
}

UTEST(OdbcCommandControl, BeginMergesHandlerAndPartialExplicitFields) {
    auto cluster = MakeCluster();
    cluster.SetDefaultCommandControl({.network_timeout = 1s, .statement_timeout = 1s});
    cluster.SetHandlersCommandControl({{
        "/begin-command-control",
        CommandControlByMethodMap{{"GET", {.statement_timeout = 0ms}}},
    }});
    server::request::kTaskInheritedData.Set(MakeRequestData(engine::Deadline{}, "/begin-command-control", "GET"));

    UEXPECT_THROW(cluster.Begin(ClusterHostType::kMaster), OperationInterrupted);
    UEXPECT_THROW(cluster.Begin(ClusterHostType::kMaster, CommandControl{.network_timeout = 1s}), OperationInterrupted);
    auto transaction = cluster.Begin(ClusterHostType::kMaster, CommandControl{.statement_timeout = 1s});
    transaction.Rollback();
    server::request::kTaskInheritedData.Erase();
}

UTEST(OdbcCommandControl, TransactionStoreAndPoolOutliveCluster) {
    std::optional<Transaction> transaction;
    const Query named{"SELECT 1", Query::Name{"after-cluster-destruction"}};
    {
        auto cluster = std::make_unique<
            Cluster>(settings::ODBCClusterSettings{{settings::HostSettings{kDSN, {1, 1}}}}, nullptr);
        cluster->SetDefaultCommandControl({.network_timeout = 1s, .statement_timeout = 1s});
        cluster->SetQueriesCommandControl({{"after-cluster-destruction", {.network_timeout = 0ms}}});
        transaction.emplace(cluster->Begin(ClusterHostType::kMaster));
        cluster->SetQueriesCommandControl({});
    }

    ASSERT_TRUE(transaction);
    UEXPECT_NO_THROW(transaction->Execute(named));
    transaction->Commit();
}

}  // namespace storages::odbc::tests

USERVER_NAMESPACE_END
