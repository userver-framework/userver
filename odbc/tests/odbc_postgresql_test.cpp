#include <gtest/gtest.h>
#include <cstdint>
#include <limits>
#include <optional>
#include <storages/odbc/detail/connection.hpp>
#include <storages/odbc/detail/tracing.hpp>
#include <string_view>
#include <userver/storages/odbc.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>

#include <userver/storages/odbc/tests/utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::tests {

UTEST(CreateConnection, Works) { auto cluster = MakeCluster(); }

UTEST(CreateConnection, MultipleDSN) { auto cluster = MakeCluster(kMultiDSNSettings); }

UTEST(DriverCapabilities, CapturesPsqlOdbcSnapshot) {
    Connection connection{kDSN};
    const auto& capabilities = connection.GetDriverCapabilities();

    EXPECT_EQ(capabilities.GetDbmsName(), "PostgreSQL");
    EXPECT_FALSE(capabilities.GetDbmsVersion().empty());
    EXPECT_NE(capabilities.GetDriverName().find("psqlodbc"), std::string::npos);
    EXPECT_FALSE(capabilities.GetDriverVersion().empty());
    EXPECT_FALSE(capabilities.GetDriverOdbcVersion().empty());

    const auto transaction_capability = capabilities.GetTransactionCapability();
    ASSERT_TRUE(transaction_capability);
    EXPECT_NE(*transaction_capability, detail::TransactionCapability::kNone);

    const auto isolation_options = capabilities.GetTransactionIsolationOptions();
    ASSERT_TRUE(isolation_options);
    EXPECT_NE(*isolation_options & SQL_TXN_READ_COMMITTED, 0U);
    const auto default_isolation = capabilities.GetDefaultTransactionIsolation();
    ASSERT_TRUE(default_isolation);
    EXPECT_NE(*default_isolation, 0U);
    EXPECT_EQ(*default_isolation & *isolation_options, *default_isolation);

    ASSERT_TRUE(capabilities.IsDataSourceReadOnly());
    EXPECT_FALSE(*capabilities.IsDataSourceReadOnly());
    ASSERT_TRUE(capabilities.CanDescribeParameters());
    EXPECT_FALSE(*capabilities.CanDescribeParameters());

    EXPECT_TRUE(capabilities.GetParameterArrayRowCounts());
    EXPECT_TRUE(capabilities.GetParameterArraySelects());
    EXPECT_TRUE(capabilities.GetBatchRowCount());

    const auto scroll_options = capabilities.GetScrollOptions();
    ASSERT_TRUE(scroll_options);
    EXPECT_NE(*scroll_options & SQL_SO_FORWARD_ONLY, 0U);
    EXPECT_TRUE(capabilities.GetGetDataExtensions());
    EXPECT_TRUE(capabilities.GetCursorCommitBehavior());
    EXPECT_TRUE(capabilities.GetCursorRollbackBehavior());
}

UTEST(Query, Works) {
    auto cluster = MakeCluster();

    auto result = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
    EXPECT_EQ(result.Size(), 1);
    EXPECT_FALSE(result.IsEmpty());
    auto row = result[0];
    EXPECT_EQ(row.Size(), 1);
    EXPECT_FALSE(row[0].IsNull());
    EXPECT_EQ(row[0].GetInt32(), 1);

    auto multipleRows = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT generate_series(1, 10)");
    EXPECT_EQ(multipleRows.Size(), 10);
    for (std::size_t i = 0; i < multipleRows.Size(); i++) {
        auto row = multipleRows[i];
        EXPECT_EQ(row.Size(), 1);
        EXPECT_FALSE(row[0].IsNull());
        EXPECT_EQ(row[0].GetInt32(), i + 1);
    }
}

UTEST(Query, BindsParametersWithoutInterpolation) {
    auto cluster = MakeCluster();

    /// [ODBC parameter binding]
    const std::string untrusted_value = "Robert'); DROP TABLE users;--";
    const auto result = cluster.Execute(
        storages::odbc::ClusterHostType::kMaster,
        "SELECT ?::text, ?::text, ?::bigint, ?::bigint, ?::double precision, ?::boolean, ?::boolean",
        untrusted_value,
        std::string_view{""},
        std::int16_t{-42},
        std::uint32_t{42},
        1.25F,
        true,
        false
    );
    /// [ODBC parameter binding]

    ASSERT_EQ(result.Size(), 1);
    EXPECT_EQ(result[0][0].GetString(), untrusted_value);
    EXPECT_EQ(result[0][1].GetString(), "");
    EXPECT_EQ(result[0][2].GetInt64(), -42);
    EXPECT_EQ(result[0][3].GetInt64(), 42);
    EXPECT_DOUBLE_EQ(result[0][4].GetDouble(), 1.25);
    EXPECT_TRUE(result[0][5].GetBool());
    EXPECT_FALSE(result[0][6].GetBool());
}

UTEST(Query, RejectsUnsignedValuesOutsidePortableBigintRange) {
    auto cluster = MakeCluster();

    const auto supported = cluster.Execute(
        storages::odbc::ClusterHostType::kMaster,
        "SELECT ?::bigint",
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())
    );
    ASSERT_EQ(supported.Size(), 1);
    EXPECT_EQ(supported[0][0].GetInt64(), std::numeric_limits<std::int64_t>::max());

    UEXPECT_THROW(
        cluster.Execute(
            storages::odbc::ClusterHostType::kMaster,
            "SELECT ?::numeric",
            std::numeric_limits<std::uint64_t>::max()
        ),
        storages::odbc::StatementError
    );
}

UTEST(Query, HonorsQueryLogModeAndName) {
    const Query named{
        "SELECT 'must not leak'",
        Query::Name{"safe_query_name"},
        Query::LogMode::kNameOnly,
    };
    const auto named_tags = detail::tracing::MakeQuerySpanTags(named);
    ASSERT_TRUE(named_tags.statement_name);
    EXPECT_EQ(*named_tags.statement_name, "safe_query_name");
    EXPECT_FALSE(named_tags.statement);

    const Query unnamed_name_only{
        "SELECT 'must not leak either'",
        std::nullopt,
        Query::LogMode::kNameOnly,
    };
    const auto hidden_tags = detail::tracing::MakeQuerySpanTags(unnamed_name_only);
    EXPECT_FALSE(hidden_tags.statement_name);
    EXPECT_FALSE(hidden_tags.statement);

    const Query unnamed_full{"SELECT 1"};
    const auto full_tags = detail::tracing::MakeQuerySpanTags(unnamed_full);
    EXPECT_FALSE(full_tags.statement_name);
    ASSERT_TRUE(full_tags.statement);
    EXPECT_EQ(*full_tags.statement, "SELECT 1");
}

UTEST(Query, BindsTypedNull) {
    auto cluster = MakeCluster();

    const std::optional<std::string> value;
    const auto result = cluster.Execute(
        storages::odbc::ClusterHostType::kMaster,
        "SELECT ?::text IS NULL, ?::text IS NULL",
        value,
        nullptr
    );

    ASSERT_EQ(result.Size(), 1);
    EXPECT_TRUE(result[0][0].GetBool());
    EXPECT_TRUE(result[0][1].GetBool());
}

UTEST(Query, ParameterCountMismatchIsStatementError) {
    auto cluster = MakeCluster();
    UEXPECT_THROW(
        cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT ?::integer", 1, 2),
        storages::odbc::StatementError
    );
}

UTEST(Query, VariousTypes) {
    auto query = "SELECT 42, 'test', 1.0, false, null, true";
    auto cluster = MakeCluster();

    auto result = cluster.Execute(storages::odbc::ClusterHostType::kMaster, query);
    EXPECT_EQ(result.Size(), 1);
    EXPECT_FALSE(result.IsEmpty());
    auto row = result[0];
    EXPECT_EQ(row.Size(), 6);

    auto intField = row[0];
    EXPECT_EQ(intField.GetInt32(), 42);

    auto stringField = row[1];
    EXPECT_EQ(stringField.GetString(), "test");

    auto doubleField = row[2];
    EXPECT_DOUBLE_EQ(doubleField.GetDouble(), 1.0);

    auto boolField = row[3];
    EXPECT_EQ(boolField.GetBool(), false);

    auto nullField = row[4];
    EXPECT_TRUE(nullField.IsNull());

    auto trueBool = row[5];
    EXPECT_EQ(trueBool.GetBool(), true);
}

UTEST(Query, DifferentHostTypes) {
    auto query = "SELECT 1";
    auto cluster = MakeCluster();

    // TODO: needs an actual check that host are selected correctly
    cluster.Execute(storages::odbc::ClusterHostType::kMaster, query);
    cluster.Execute(storages::odbc::ClusterHostType::kSlave, query);
    cluster.Execute(storages::odbc::ClusterHostType::kNone, query);
}

UTEST(Query, EmptyResult) {
    auto cluster = MakeCluster();
    auto result = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1 WHERE false");
    EXPECT_EQ(result.Size(), 0);
    EXPECT_TRUE(result.IsEmpty());
}

UTEST(Query, FieldCount) {
    auto cluster = MakeCluster();
    auto result = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1, 2, 3");
    EXPECT_EQ(result.FieldCount(), 3);
    EXPECT_EQ(result.Size(), 1);
    EXPECT_EQ(result[0].Size(), 3);
}

UTEST(Query, GetInt64) {
    auto cluster = MakeCluster();
    auto result = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 2147483648");
    EXPECT_EQ(result[0][0].GetInt64(), 2147483648LL);
}

UTEST(Query, GetStringFromNumber) {
    auto cluster = MakeCluster();
    auto result = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT '42'");
    EXPECT_EQ(result[0][0].GetString(), "42");
}

UTEST(Query, MaterializedResultOutlivesConnectionAndSubsequentQuery) {
    const auto host_settings = storages::odbc::settings::HostSettings{kDSN, {1, 1}};
    std::optional<ResultSet> saved_result;
    {
        storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{host_settings}}, nullptr);
        saved_result
            .emplace(cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT ?::text AS saved_value", "first")
            );

        const auto second = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT ?::integer", 2);
        ASSERT_EQ(second.Size(), 1);
        EXPECT_EQ(second[0][0].GetInt32(), 2);
    }

    ASSERT_TRUE(saved_result);
    ASSERT_EQ(saved_result->Size(), 1);
    EXPECT_EQ(saved_result->GetFieldName(0), "saved_value");
    EXPECT_EQ((*saved_result)[0][0].GetString(), "first");
}

UTEST(Query, MaterializesLongEmptyAndNullValues) {
    auto cluster = MakeCluster();
    const std::string long_value(70'000, 'x');

    const auto result = cluster.Execute(
        storages::odbc::ClusterHostType::kMaster,
        "SELECT ?::text AS long_value, ?::text AS empty_value, NULL::text AS null_value",
        long_value,
        std::string{}
    );

    ASSERT_EQ(result.Size(), 1);
    EXPECT_EQ(result[0][0].GetString(), long_value);
    EXPECT_EQ(result[0][1].GetString(), "");
    EXPECT_TRUE(result[0][2].IsNull());
    UEXPECT_THROW(result[0][2].GetString(), storages::odbc::ResultSetError);
}

UTEST(Query, MaterializesChunkBoundariesAndTypedNulls) {
    auto cluster = MakeCluster();
    const std::string value_4095(4095, 'a');
    const std::string value_4096(4096, 'b');
    const std::string value_4097(4097, 'c');
    const std::string value_65537(65'537, 'd');

    const auto result = cluster.Execute(
        storages::odbc::ClusterHostType::kMaster,
        "SELECT ?::text, ?::text, ?::text, ?::text, NULL::integer, NULL::boolean, NULL::double precision",
        value_4095,
        value_4096,
        value_4097,
        value_65537
    );

    ASSERT_EQ(result.Size(), 1);
    EXPECT_EQ(result[0][0].GetString(), value_4095);
    EXPECT_EQ(result[0][1].GetString(), value_4096);
    EXPECT_EQ(result[0][2].GetString(), value_4097);
    EXPECT_EQ(result[0][3].GetString(), value_65537);
    for (std::size_t index = 4; index < 7; ++index) {
        EXPECT_TRUE(result[0][index].IsNull());
    }
    UEXPECT_THROW(result[0][4].GetInt32(), storages::odbc::ResultSetError);
    UEXPECT_THROW(result[0][5].GetBool(), storages::odbc::ResultSetError);
    UEXPECT_THROW(result[0][6].GetDouble(), storages::odbc::ResultSetError);
}

UTEST(Query, MaterializedResultSurvivesTopologyReload) {
    const auto host_settings = storages::odbc::settings::HostSettings{kDSN, {1, 1}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{host_settings}}, nullptr);
    const auto result = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT ?::text", "before reload");

    const std::string reloaded_dsn = std::string{kDSN} + "ApplicationName=odbc-materialized-result;";
    cluster.UpdateSettings(storages::odbc::settings::ODBCClusterSettings{{
        storages::odbc::settings::HostSettings{reloaded_dsn, {1, 1}},
    }});

    ASSERT_EQ(result.Size(), 1);
    EXPECT_EQ(result[0][0].GetString(), "before reload");
}

UTEST(Query, SeparatesRowsFromRowsAffected) {
    const auto host_settings = storages::odbc::settings::HostSettings{kDSN, {1, 1}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{host_settings}}, nullptr);
    cluster.Execute(storages::odbc::ClusterHostType::kMaster, "CREATE TEMP TABLE odbc_rows_affected(value INTEGER)");

    const auto insert = cluster.Execute(
        storages::odbc::ClusterHostType::kMaster,
        "INSERT INTO odbc_rows_affected(value) VALUES (?), (?)",
        1,
        2
    );
    EXPECT_EQ(insert.Size(), 0);
    EXPECT_EQ(insert.RowsAffected(), 2);

    const auto select =
        cluster
            .Execute(storages::odbc::ClusterHostType::kMaster, "SELECT value FROM odbc_rows_affected ORDER BY value");
    EXPECT_EQ(select.Size(), 2);
    EXPECT_EQ(select.RowsAffected(), 0);
}

UTEST(Pool, LessQueriesThanConnections) {
    std::size_t poolConnections = 5;
    auto hostSettings = storages::odbc::settings::HostSettings{kDSN, {poolConnections, poolConnections}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{hostSettings}}, nullptr);

    std::vector<engine::TaskWithResult<ResultSet>> futures;
    futures.reserve(poolConnections);

    for (std::size_t i = 0; i < poolConnections - 1; i++) {
        futures.emplace_back(utils::Async("LessQueriesThanConnections", [&cluster]() {
            return cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
        }));
    }

    for (auto& future : futures) {
        auto result = future.Get();
        EXPECT_EQ(result.Size(), 1);
        EXPECT_EQ(result[0][0].GetInt32(), 1);
    }
}

UTEST(Pool, EqualQueriesAndConnections) {
    std::size_t poolConnections = 5;
    auto hostSettings = storages::odbc::settings::HostSettings{kDSN, {poolConnections, poolConnections}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{hostSettings}}, nullptr);

    std::vector<engine::TaskWithResult<ResultSet>> futures;
    futures.reserve(poolConnections);

    for (std::size_t i = 0; i < poolConnections; i++) {
        futures.emplace_back(utils::Async("EqualQueriesAndConnections", [&cluster]() {
            return cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
        }));
    }

    for (auto& future : futures) {
        auto result = future.Get();
        EXPECT_EQ(result.Size(), 1);
        EXPECT_EQ(result[0][0].GetInt32(), 1);
    }
}

UTEST(Pool, MoreQueriesThanConnectionsButLessThanPoolSize) {
    std::size_t poolConnections = 5;
    auto hostSettings = storages::odbc::settings::HostSettings{kDSN, {poolConnections, poolConnections * 2}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{hostSettings}}, nullptr);

    std::vector<engine::TaskWithResult<ResultSet>> futures;
    futures.reserve(poolConnections + 2);

    for (std::size_t i = 0; i < poolConnections + 2; i++) {
        futures.emplace_back(utils::Async("MoreQueriesThanConnectionsButLessThanPoolSize", [&cluster]() {
            return cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
        }));
    }

    for (auto& future : futures) {
        auto result = future.Get();
        EXPECT_EQ(result.Size(), 1);
        EXPECT_EQ(result[0][0].GetInt32(), 1);
    }
}

UTEST(Pool, MoreQueriesThanConnectionsAndPoolSize) {
    std::size_t poolConnections = 5;
    auto hostSettings = storages::odbc::settings::HostSettings{kDSN, {poolConnections, poolConnections}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{hostSettings}}, nullptr);

    std::vector<engine::TaskWithResult<ResultSet>> futures;
    futures.reserve(poolConnections * 2);

    for (std::size_t i = 0; i < poolConnections * 2; i++) {
        futures.emplace_back(utils::Async("MoreQueriesThanConnectionsAndPoolSize", [&cluster]() {
            return cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
        }));
    }

    for (auto& future : futures) {
        auto result = future.Get();
        EXPECT_EQ(result.Size(), 1);
        EXPECT_EQ(result[0][0].GetInt32(), 1);
    }
}

UTEST(Pool, RestoresBrokenConnection) {
    auto hostSettings = storages::odbc::settings::HostSettings{kDSN, {1, 1}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{hostSettings}}, nullptr);

    auto killConnectionQuery = "SELECT pg_terminate_backend(pid) FROM pg_stat_activity WHERE datname = 'postgres';";

    try {
        cluster.Execute(storages::odbc::ClusterHostType::kMaster, killConnectionQuery);
    } catch (...) {
    }

    auto selectRes = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
    EXPECT_EQ(selectRes.Size(), 1);
    EXPECT_EQ(selectRes[0][0].GetInt32(), 1);
}

}  // namespace storages::odbc::tests

USERVER_NAMESPACE_END
