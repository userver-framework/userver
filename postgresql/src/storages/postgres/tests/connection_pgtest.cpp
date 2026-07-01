#include <storages/postgres/tests/util_pgtest.hpp>

#include <storages/postgres/detail/connection.hpp>
#include <userver/storages/postgres/dsn.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/storages/postgres/null.hpp>
#include <userver/utils/statistics/metrics_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;

namespace static_test {

struct NoInputOperator {};
static_assert(!pg::io::traits::HasParser<NoInputOperator>, "Test has parser metafunction");
static_assert(pg::io::traits::HasParser<int>, "Test has parser metafunction");

}  // namespace static_test

namespace {

UTEST_P(PostgreConnection, SelectOne) {
    CheckConnection(GetConn());

    pg::ResultSet res{nullptr};
    UEXPECT_NO_THROW(res = GetConn()->Execute("select 1 as val")) << "select 1 successfully executes";
    EXPECT_EQ(pg::ConnectionState::kIdle, GetConn()->GetState());
    EXPECT_FALSE(res.IsEmpty()) << "Result set is obtained";
    EXPECT_EQ(1, res.Size()) << "Result contains 1 row";
    EXPECT_EQ(1, res.FieldCount()) << "Result contains 1 field";

    for (const auto& row : res) {
        EXPECT_EQ(1, row.Size()) << "Row contains 1 field";
        pg::Integer val{0};
        UEXPECT_NO_THROW(row.To(val)) << "Extract row data";
        EXPECT_EQ(1, val) << "Correct data extracted";
        UEXPECT_NO_THROW(val = row["val"].As<pg::Integer>()) << "Access field by name";
        EXPECT_EQ(1, val) << "Correct data extracted";
        for (const auto& field : row) {
            EXPECT_FALSE(field.IsNull()) << "Field is not null";
            EXPECT_EQ(1, field.As<pg::Integer>()) << "Correct data extracted";
        }
    }
}

UTEST_P(PostgreConnection, SelectPlaceholder) {
    CheckConnection(GetConn());

    pg::ResultSet res{nullptr};
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1", 42)) << "select integral placeholder successfully executes";
    EXPECT_EQ(pg::ConnectionState::kIdle, GetConn()->GetState());
    EXPECT_FALSE(res.IsEmpty()) << "Result set is obtained";
    EXPECT_EQ(1, res.Size()) << "Result contains 1 row";
    EXPECT_EQ(1, res.FieldCount()) << "Result contains 1 field";

    for (const auto& row : res) {
        EXPECT_EQ(1, row.Size()) << "Row contains 1 field";
        for (const auto& field : row) {
            EXPECT_FALSE(field.IsNull()) << "Field is not null";
            EXPECT_EQ(42, field.As<pg::Integer>());
        }
    }

    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1", "fooo")) << "select text placeholder successfully executes";
    EXPECT_EQ(pg::ConnectionState::kIdle, GetConn()->GetState());
    EXPECT_FALSE(res.IsEmpty()) << "Result set is obtained";
    EXPECT_EQ(1, res.Size()) << "Result contains 1 row";
    EXPECT_EQ(1, res.FieldCount()) << "Result contains 1 field";

    for (const auto& row : res) {
        EXPECT_EQ(1, row.Size()) << "Row contains 1 field";
        for (const auto& field : row) {
            EXPECT_FALSE(field.IsNull()) << "Field is not null";
            EXPECT_EQ("fooo", field.As<std::string>());
        }
    }
}

UTEST_P(PostgreConnection, CheckResultset) {
    CheckConnection(GetConn());

    pg::ResultSet res{nullptr};
    UEXPECT_NO_THROW(
        res = GetConn()->Execute("select $1 as str, $2 as int, $3 as float, $4 as double", "foo bar", 42, 3.14F, 6.28)
    ) << "select four cols successfully executes";
    EXPECT_EQ(pg::ConnectionState::kIdle, GetConn()->GetState());
    EXPECT_FALSE(res.IsEmpty()) << "Result set is obtained";
    EXPECT_EQ(1, res.Size()) << "Result contains 1 row";
    EXPECT_EQ(4, res.FieldCount()) << "Result contains 4 fields";
    EXPECT_EQ(1, res.RowsAffected()) << "The query affected 1 row";
    EXPECT_EQ("SELECT 1", res.CommandStatus());

    for (const auto& row : res) {
        EXPECT_EQ(4, row.Size()) << "Row contains 4 fields";
        {
            std::string str;
            pg::Integer i{};
            float f{};
            double d{};
            UEXPECT_NO_THROW(row.To(str, i, f, d));
            EXPECT_EQ("foo bar", str);
            EXPECT_EQ(42, i);
            EXPECT_EQ(3.14F, f);
            EXPECT_EQ(6.28, d);
        }
        {
            std::string str;
            pg::Integer i{};
            float f{};
            double d{};
            UEXPECT_NO_THROW(row.To({"int", "str", "double", "float"}, i, str, d, f));
            EXPECT_EQ("foo bar", str);
            EXPECT_EQ(42, i);
            EXPECT_EQ(3.14F, f);
            EXPECT_EQ(6.28, d);
        }
        {
            std::string str;
            pg::Integer i{};
            float f{};
            double d{};
            UEXPECT_NO_THROW(row.To({1, 0, 3, 2}, i, str, d, f));
            EXPECT_EQ("foo bar", str);
            EXPECT_EQ(42, i);
            EXPECT_EQ(3.14F, f);
            EXPECT_EQ(6.28, d);
        }
        {
            std::string str;
            pg::Integer i{};
            float f{};
            double d{};
            UEXPECT_NO_THROW((std::tie(str, i, f, d) = row.As<std::string, pg::Integer, float, double>()));
            EXPECT_EQ("foo bar", str);
            EXPECT_EQ(42, i);
            EXPECT_EQ(3.14F, f);
            EXPECT_EQ(6.28, d);
        }
        {
            auto [str, i, f, d] = row.As<std::string, pg::Integer, float, double>();
            EXPECT_EQ("foo bar", str);
            EXPECT_EQ(42, i);
            EXPECT_EQ(3.14F, f);
            EXPECT_EQ(6.28, d);
        }
        {
            auto [str, d] = row.As<std::string, double>({"str", "double"});
            EXPECT_EQ("foo bar", str);
            EXPECT_EQ(6.28, d);
        }
        {
            auto [str, d] = row.As<std::string, double>({0, 3});
            EXPECT_EQ("foo bar", str);
            EXPECT_EQ(6.28, d);
        }
    }
}

UTEST_P(PostgreConnection, QueryErrors) {
    CheckConnection(GetConn());
    pg::ResultSet res{nullptr};
    const std::string temp_table = R"~(
      create temporary table pgtest(
        id integer primary key,
        nn_val integer not null,
        check_val integer check(check_val > 0))
      )~";
    const std::string dependent_table = R"~(
      create temporary table dependent(
        id integer references pgtest(id) on delete restrict)
      )~";
    const std::string insert_pgtest = "insert into pgtest(id, nn_val, check_val) values ($1, $2, $3)";

    UEXPECT_THROW(res = GetConn()->Execute("elect"), pg::SyntaxError);
    UEXPECT_THROW(res = GetConn()->Execute("select foo"), pg::AccessRuleViolation);
    UEXPECT_THROW(res = GetConn()->Execute(""), pg::LogicError);

    UEXPECT_NO_THROW(GetConn()->Execute(temp_table));
    UEXPECT_NO_THROW(GetConn()->Execute(dependent_table));
    UEXPECT_THROW(GetConn()->Execute(insert_pgtest, 1, pg::null<int>, pg::null<int>), pg::NotNullViolation);
    UEXPECT_NO_THROW(GetConn()->Execute(insert_pgtest, 1, 1, pg::null<int>));
    UEXPECT_THROW(GetConn()->Execute(insert_pgtest, 1, 1, pg::null<int>), pg::UniqueViolation);
    UEXPECT_THROW(GetConn()->Execute(insert_pgtest, 2, 1, 0), pg::CheckViolation);
    UEXPECT_THROW(GetConn()->Execute("insert into dependent values(3)"), pg::ForeignKeyViolation);
    UEXPECT_NO_THROW(GetConn()->Execute("insert into dependent values(1)"));
    UEXPECT_THROW(GetConn()->Execute("delete from pgtest where id = 1"), pg::ForeignKeyViolation);
}

UTEST_P(PostgreConnection, DuplicatePreparedStatement) {
    CheckConnection(GetConn());

    UEXPECT_NO_THROW(GetConn()->Execute("prepare pgtest_stmt as select 1"));
    UEXPECT_THROW(GetConn()->Execute("prepare pgtest_stmt as select 1"), pg::DuplicatePreparedStatement);
    EXPECT_EQ(pg::ConnectionState::kIdle, GetConn()->GetState()) << "Connection is not broken";
    UEXPECT_NO_THROW(GetConn()->Execute("select 1")) << "Connection is still usable";

    UEXPECT_NO_THROW(GetConn()->Begin({}, {}));
    UEXPECT_NO_THROW(GetConn()->Execute("prepare pgtest_stmt_trx as select 1"));
    UEXPECT_THROW(GetConn()->Execute("prepare pgtest_stmt_trx as select 1"), pg::DuplicatePreparedStatement);
    EXPECT_EQ(pg::ConnectionState::kTranError, GetConn()->GetState());
    UEXPECT_NO_THROW(GetConn()->Rollback());
    EXPECT_EQ(pg::ConnectionState::kIdle, GetConn()->GetState());
}

UTEST_P(PostgreConnection, InvalidParameter) {
    CheckConnection(GetConn());
    UEXPECT_THROW(
        {
            GetConn()->SetParameter("invalid", "parameter", pg::detail::Connection::ParameterScope::kSession);
            auto res = GetConn()->Execute("select 1");
        },
        pg::AccessRuleViolation
    );
}

UTEST_P(PostgreConnection, ManualTransaction) {
    CheckConnection(GetConn());
    EXPECT_EQ(pg::ConnectionState::kIdle, GetConn()->GetState());
    UEXPECT_NO_THROW(GetConn()->Execute("begin")) << "Successfully execute begin statement";
    EXPECT_EQ(pg::ConnectionState::kTranIdle, GetConn()->GetState());
    UEXPECT_NO_THROW(GetConn()->Execute("commit")) << "Successfully execute commit statement";
    EXPECT_EQ(pg::ConnectionState::kIdle, GetConn()->GetState());
}

UTEST_P(PostgreConnection, AutoTransaction) {
    CheckConnection(GetConn());
    pg::ResultSet res{nullptr};

    EXPECT_EQ(pg::ConnectionState::kIdle, GetConn()->GetState());
    {
        pg::Transaction trx(std::move(GetConn()), pg::TransactionOptions{});
        // TODO Delegate state to transaction and test it
        //    EXPECT_EQ(pg::ConnectionState::kTranIdle, GetConn()->GetState());
        //    EXPECT_TRUE(GetConn()->IsInTransaction());
        //    UEXPECT_THROW(GetConn()->Begin(pg::TransactionOptions{}, cb),
        //                  pg::AlreadyInTransaction);

        UEXPECT_NO_THROW(res = trx.Execute("select 1"));
        //    EXPECT_EQ(pg::ConnectionState::kTranIdle, GetConn()->GetState());
        EXPECT_FALSE(res.IsEmpty()) << "Result set is obtained";

        UEXPECT_NO_THROW(trx.Commit());

        UEXPECT_THROW(trx.Commit(), pg::NotInTransaction);
        UEXPECT_NO_THROW(trx.Rollback());
    }
}

UTEST_P(PostgreConnection, RAIITransaction) {
    CheckConnection(GetConn());
    pg::ResultSet res{nullptr};

    EXPECT_EQ(pg::ConnectionState::kIdle, GetConn()->GetState());
    {
        pg::Transaction trx(std::move(GetConn()), pg::TransactionOptions{});
        // TODO Delegate state to transaction and test it
        //    EXPECT_EQ(pg::ConnectionState::kTranIdle, GetConn()->GetState());
        //    EXPECT_TRUE(GetConn()->IsInTransaction());

        UEXPECT_NO_THROW(res = trx.Execute("select 1"));
        //    EXPECT_EQ(pg::ConnectionState::kTranIdle, GetConn()->GetState());
        EXPECT_FALSE(res.IsEmpty()) << "Result set is obtained";
    }
}

void CheckTransactionIsolationLevel(
    pg::detail::ConnectionPtr& conn,
    pg::TransactionOptions options,
    pg::IsolationLevel lvl
) {
    EXPECT_EQ(pg::ConnectionState::kIdle, conn->GetState());
    pg::Transaction trx(std::move(conn), options);
    pg::ResultSet res{nullptr};
    UEXPECT_NO_THROW(res = trx.Execute("SELECT current_setting('transaction_isolation')"));
    EXPECT_FALSE(res.IsEmpty()) << "Result set is obtained";
    EXPECT_EQ(ToStringView(lvl), res.AsSingleRow<std::string>());
}

UTEST_P(PostgreConnection, TransactionRepeatableReadIsolationLevel) {
    CheckConnection(GetConn());
    CheckTransactionIsolationLevel(GetConn(), pg::Transaction::RepeatableReadRO, pg::IsolationLevel::kRepeatableRead);
}

UTEST_P(PostgreConnection, TransactionSerializableIsolationLevel) {
    CheckConnection(GetConn());
    CheckTransactionIsolationLevel(GetConn(), pg::Transaction::SerializableRO, pg::IsolationLevel::kSerializable);
}

UTEST_P(PostgreConnection, RollbackToIdle) {
    CheckConnection(GetConn());
    EXPECT_EQ(pg::ConnectionState::kIdle, GetConn()->GetState());
    UEXPECT_NO_THROW(GetConn()->Begin({}, {}));
    // at this point transaction could be either kTranIdle or kTranActive,
    // depending on the pipeline mode setting
    UEXPECT_NO_THROW(GetConn()->Rollback());
    EXPECT_EQ(pg::ConnectionState::kIdle, GetConn()->GetState());
}

UTEST_P(PostgreConnection, RollbackOnBusyOeErroredConnection) {
    CheckConnection(GetConn());

    EXPECT_EQ(pg::ConnectionState::kIdle, GetConn()->GetState());
    UEXPECT_NO_THROW(GetConn()->Begin({}, {}));
    // Network timeout
    const DefaultCommandControlScope
        scope(pg::CommandControl{std::chrono::milliseconds{10}, std::chrono::milliseconds{0}});
    UEXPECT_THROW(GetConn()->Execute("select pg_sleep(1)"), pg::ConnectionTimeoutError);
    EXPECT_EQ(pg::ConnectionState::kTranActive, GetConn()->GetState());
    UEXPECT_NO_THROW(GetConn()->Rollback());
    UEXPECT_NO_THROW(GetConn()->CancelAndCleanup(utest::kMaxTestWaitTime));
    EXPECT_EQ(pg::ConnectionState::kIdle, GetConn()->GetState());
    EXPECT_FALSE(GetConn()->IsBroken());
    // Query cancelled
    const DefaultCommandControlScope scope2(pg::CommandControl{std::chrono::seconds{2}, std::chrono::milliseconds{200}}
    );
    UEXPECT_NO_THROW(GetConn()->Begin({}, {}));
    UEXPECT_THROW(GetConn()->Execute("select pg_sleep(1.5)"), pg::QueryCancelled);
    EXPECT_EQ(pg::ConnectionState::kTranError, GetConn()->GetState());
    UEXPECT_NO_THROW(GetConn()->Rollback());
    UEXPECT_NO_THROW(GetConn()->CancelAndCleanup(utest::kMaxTestWaitTime));
    EXPECT_EQ(pg::ConnectionState::kIdle, GetConn()->GetState());
    EXPECT_FALSE(GetConn()->IsBroken());
}

UTEST_P(PostgreConnection, CommitOnBusyOeErroredConnection) {
    CheckConnection(GetConn());

    EXPECT_EQ(pg::ConnectionState::kIdle, GetConn()->GetState());
    UEXPECT_NO_THROW(GetConn()->Begin({}, {}));
    // Network timeout
    const DefaultCommandControlScope
        scope(pg::CommandControl{std::chrono::milliseconds{10}, std::chrono::milliseconds{0}});
    UEXPECT_THROW(GetConn()->Execute("select pg_sleep(1)"), pg::ConnectionTimeoutError);
    EXPECT_EQ(pg::ConnectionState::kTranActive, GetConn()->GetState());
    UEXPECT_THROW(GetConn()->Commit(), std::exception);
    UEXPECT_NO_THROW(GetConn()->CancelAndCleanup(utest::kMaxTestWaitTime));
    EXPECT_EQ(pg::ConnectionState::kIdle, GetConn()->GetState());
    EXPECT_FALSE(GetConn()->IsBroken());
    // Query cancelled
    const DefaultCommandControlScope scope2(pg::CommandControl{std::chrono::seconds{2}, std::chrono::milliseconds{200}}
    );
    UEXPECT_NO_THROW(GetConn()->Begin({}, {}));
    UEXPECT_THROW(GetConn()->Execute("select pg_sleep(1.5)"), pg::QueryCancelled);
    EXPECT_EQ(pg::ConnectionState::kTranError, GetConn()->GetState());

    // Server automatically replaces COMMIT with a ROLLBACK for aborted txns
    UEXPECT_THROW(GetConn()->Commit(), std::exception);

    EXPECT_EQ(pg::ConnectionState::kIdle, GetConn()->GetState());
    EXPECT_FALSE(GetConn()->IsBroken());
}

UTEST_P(PostgreConnection, StatementTimeout) {
    CheckConnection(GetConn());

    EXPECT_EQ(pg::ConnectionState::kIdle, GetConn()->GetState());
    // Network timeout
    const DefaultCommandControlScope
        scope(pg::CommandControl{std::chrono::milliseconds{10}, std::chrono::milliseconds{0}});
    UEXPECT_THROW(GetConn()->Execute("select pg_sleep(1)"), pg::ConnectionTimeoutError);
    EXPECT_EQ(pg::ConnectionState::kTranActive, GetConn()->GetState());
    UEXPECT_NO_THROW(GetConn()->CancelAndCleanup(utest::kMaxTestWaitTime));
    EXPECT_EQ(pg::ConnectionState::kIdle, GetConn()->GetState());
    EXPECT_FALSE(GetConn()->IsBroken());
    // Query cancelled
    const DefaultCommandControlScope scope2(pg::CommandControl{std::chrono::seconds{2}, std::chrono::milliseconds{200}}
    );
    UEXPECT_THROW(GetConn()->Execute("select pg_sleep(1.5)"), pg::QueryCancelled);
    EXPECT_EQ(pg::ConnectionState::kIdle, GetConn()->GetState());
    UEXPECT_NO_THROW(GetConn()->CancelAndCleanup(utest::kMaxTestWaitTime));
    EXPECT_EQ(pg::ConnectionState::kIdle, GetConn()->GetState());
    EXPECT_FALSE(GetConn()->IsBroken());
}

UTEST_P(PostgreConnection, CachedPlanChange) {
    // this only works with english messages, better than nothing
    GetConn()->Execute("SET lc_messages = 'en_US.UTF-8'");
    GetConn()->Execute("CREATE TEMPORARY TABLE plan_change_test ( a integer )");
    UEXPECT_NO_THROW(GetConn()->Execute("SELECT * FROM plan_change_test"));
    GetConn()->Execute("ALTER TABLE plan_change_test ALTER a TYPE bigint");
    UEXPECT_THROW(GetConn()->Execute("SELECT * FROM plan_change_test"), pg::FeatureNotSupported);
    // broken plan should not be reused anymore
    UEXPECT_NO_THROW(GetConn()->Execute("SELECT * FROM plan_change_test"));
}

}  // namespace

class PostgreCustomConnection : public PostgreSQLBase {};

UTEST_F(PostgreCustomConnection, Connect) {
    UEXPECT_THROW(
        pg::detail::Connection::Connect(
            pg::Dsn{"psql://"},
            nullptr,
            GetTaskProcessor(),
            GetTaskStorage(),
            kConnectionId,
            kCachePreparedStatements,
            GetTestCmdCtls(),
            {},
            {},
            {},
            std::make_shared<utils::statistics::MetricsStorage>()
        ),
        pg::InvalidDSN
    ) << "Connected with invalid DSN";

    MakeConnection(GetDsnFromEnv(), GetTaskProcessor());
}

UTEST_F(PostgreCustomConnection, NoPreparedStatements) {
    UEXPECT_NO_THROW(MakeConnection(GetDsnFromEnv(), GetTaskProcessor(), kNoPreparedStatements));
}

UTEST_F(PostgreCustomConnection, PreparedStatementsOverrideEnabled) {
    pg::detail::ConnectionPtr conn{nullptr};
    UEXPECT_NO_THROW(conn = MakeConnection(GetDsnFromEnv(), GetTaskProcessor(), kNoPreparedStatements));
    ASSERT_TRUE(conn);
    const auto old_stats = conn->GetStatsAndReset();
    auto cc = pg::CommandControl{
        std::chrono::milliseconds{100},
        std::chrono::milliseconds{10},
        pg::CommandControl::PreparedStatementsOptionOverride::kEnabled
    };
    UEXPECT_NO_THROW(conn->Execute("select 1", {}, cc));
    const auto stats = conn->GetStatsAndReset();
    EXPECT_GT(stats.prepared_statements_current, old_stats.prepared_statements_current);
}

UTEST_F(PostgreCustomConnection, PreparedStatementsOverrideDisabled) {
    pg::detail::ConnectionPtr conn{nullptr};
    UEXPECT_NO_THROW(conn = MakeConnection(GetDsnFromEnv(), GetTaskProcessor(), kCachePreparedStatements));
    ASSERT_TRUE(conn);
    const auto old_stats = conn->GetStatsAndReset();
    auto cc = pg::CommandControl{
        std::chrono::milliseconds{100},
        std::chrono::milliseconds{10},
        pg::CommandControl::PreparedStatementsOptionOverride::kDisabled
    };
    UEXPECT_NO_THROW(conn->Execute("select 1", {}, cc));
    const auto stats = conn->GetStatsAndReset();
    EXPECT_EQ(stats.prepared_statements_current, old_stats.prepared_statements_current);
}

UTEST_F(PostgreCustomConnection, NamedPreparedStatementEvictionWithSpecialChars) {
    storages::postgres::ConnectionSettings settings = kCachePreparedStatements;
    settings.max_prepared_cache_size = storages::postgres::kMinPreparedStatementsCacheSize;

    pg::detail::ConnectionPtr conn{nullptr};
    UASSERT_NO_THROW(conn = MakeConnection(GetDsnFromEnv(), GetTaskProcessor(), settings));
    CheckConnection(conn);

    if (!conn->ArePreparedStatementsEnabled()) {
        return;
    }

    const pg::Query queries_list[] = {
        pg::Query{"SELECT 0", pg::Query::Name{"query-name-0"}},
        pg::Query{"SELECT 1", pg::Query::Name{"query-name-1"}},
        pg::Query{"SELECT 2", pg::Query::Name{"query-name-2"}},
        pg::Query{"SELECT 3", pg::Query::Name{"query-name-3"}},
        pg::Query{"SELECT 4", pg::Query::Name{"query-name-4"}},
    };
    static_assert(std::size(queries_list) > storages::postgres::kMinPreparedStatementsCacheSize);
    for (const auto& query : queries_list) {
        UEXPECT_NO_THROW(conn->Execute(query));
    }
}

UTEST_F(PostgreCustomConnection, NamedPreparedStatementEvictionWithSpecialCharsAndSameName) {
    storages::postgres::ConnectionSettings settings = kCachePreparedStatements;
    settings.max_prepared_cache_size = storages::postgres::kMinPreparedStatementsCacheSize;

    pg::detail::ConnectionPtr conn{nullptr};
    UASSERT_NO_THROW(conn = MakeConnection(GetDsnFromEnv(), GetTaskProcessor(), settings));
    CheckConnection(conn);

    if (!conn->ArePreparedStatementsEnabled()) {
        return;
    }

    const pg::Query::Name query_name{"query/name-;\"0\n!!!!"};

    // Same `query_name` is intentional!
    const std::pair<int, pg::Query> queries_list[] = {
        {0, pg::Query{"SELECT 0", query_name}},
        {1, pg::Query{"SELECT 1", query_name}},
        {2, pg::Query{"SELECT 2", query_name}},
        {3, pg::Query{"SELECT 3", query_name}},
        {4, pg::Query{"SELECT 4", query_name}},
    };
    static_assert(std::size(queries_list) > storages::postgres::kMinPreparedStatementsCacheSize);

    auto check_loop = [&](std::string_view test_description) {
        for (const auto& [index, query] : queries_list) {
            auto result = conn->Execute(query);
            EXPECT_EQ(result[0][0].As<int>(), index)
                << "Same Query::Name must still produce different IDs for prepared statements. Noted at: "
                << test_description;

            result = conn->Execute(
                // Takes up 1 place in prepared statements cache before actual execution
                "SELECT COUNT(name) FROM pg_prepared_statements "
                "WHERE name LIKE 'q_%query/name-;\"0\n!!!!'"
            );
            EXPECT_GE(result[0][0].As<int>(), 1) << test_description;
            EXPECT_LT(result[0][0].As<int>(), storages::postgres::kMinPreparedStatementsCacheSize) << test_description;
        }

        for (std::size_t i = 0; i < storages::postgres::kMinPreparedStatementsCacheSize; ++i) {
            UEXPECT_NO_THROW(conn->Execute("SELECT 999" + std::to_string(i)));
        }
        auto result = conn->Execute("SELECT COUNT(name) FROM pg_prepared_statements WHERE name LIKE 'q_%query/name-%'");
        EXPECT_EQ(result[0][0].As<int>(), 0) << "Not deallocated the prepared statement at: " << test_description;
    };

    check_loop("checking prepared statements not in transaction");

    conn->Begin({}, {});
    check_loop("checking prepared statements in transaction");
    conn->Rollback();

    conn->Begin({}, {});
    check_loop("checking prepared in transaction after rollback");
    conn->Rollback();

    check_loop("checking prepared statements not in transaction after rollback");

    conn->Begin({}, {});
    check_loop("checking prepared statements in transaction again");
    conn->Commit();

    check_loop("checking prepared statements not in transaction after commit");
}

UTEST_F(PostgreCustomConnection, NamedPreparedStatementEvictionWithMixedCase) {
    storages::postgres::ConnectionSettings settings = kCachePreparedStatements;
    settings.max_prepared_cache_size = storages::postgres::kMinPreparedStatementsCacheSize;

    pg::detail::ConnectionPtr conn{nullptr};
    UASSERT_NO_THROW(conn = MakeConnection(GetDsnFromEnv(), GetTaskProcessor(), settings));
    CheckConnection(conn);

    if (!conn->ArePreparedStatementsEnabled()) {
        return;
    }

    const pg::Query queries_list[] = {
        pg::Query{"SELECT 0", pg::Query::Name{"SelectValueV0"}},
        pg::Query{"SELECT 1", pg::Query::Name{"SelectValueV1"}},
        pg::Query{"SELECT 2", pg::Query::Name{"SelectValueV2"}},
        pg::Query{"SELECT 3", pg::Query::Name{"SelectValueV3"}},
        pg::Query{"SELECT 4", pg::Query::Name{"SelectValueV4"}},
    };
    static_assert(std::size(queries_list) > storages::postgres::kMinPreparedStatementsCacheSize);
    for (const auto& query : queries_list) {
        UEXPECT_NO_THROW(conn->Execute(query));
    }
}

UTEST_F(PostgreCustomConnection, MaxPreparedCacheSize3) {
    pg::detail::ConnectionPtr conn{nullptr};
    UEXPECT_NO_THROW(conn = MakeConnection(GetDsnFromEnv(), GetTaskProcessor(), kMaxPreparedCacheSize3));
    ASSERT_TRUE(conn);

    UEXPECT_NO_THROW(conn->Execute("select 1"));
    UEXPECT_NO_THROW(conn->Execute("create type user_type as enum ('test')"));
    UEXPECT_THROW(conn->Execute("select 'test'::user_type"), pg::NoBinaryParser);
    UEXPECT_NO_THROW(conn->Execute("drop type user_type"));
}

UTEST_F(PostgreCustomConnection, NoUserTypes) {
    pg::detail::ConnectionPtr conn{nullptr};
    UASSERT_NO_THROW(conn = MakeConnection(GetDsnFromEnv(), GetTaskProcessor(), kNoUserTypes));
    ASSERT_TRUE(conn);

    UEXPECT_NO_THROW(conn->Execute("select 1"));
    UEXPECT_NO_THROW(conn->Execute("create type user_type as enum ('test')"));
    UEXPECT_THROW(conn->Execute("select 'test'::user_type"), pg::UnknownBufferCategory);
    UEXPECT_NO_THROW(conn->Execute("drop type user_type"));
}

UTEST_F(PostgreCustomConnection, SessionModeSetsStatementTimeoutOnConnect) {
    pg::detail::ConnectionPtr conn{nullptr};
    UASSERT_NO_THROW(conn = MakeConnection(GetDsnFromEnv(), GetTaskProcessor(), kCachePreparedStatements));
    ASSERT_TRUE(conn);

    EXPECT_EQ(kTestCmdCtl.statement_timeout_ms, conn->GetStatementTimeout());
}

USERVER_NAMESPACE_END
