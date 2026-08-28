#include <gmock/gmock.h>

#include <storages/postgres/tests/util_pgtest.hpp>

#include <storages/postgres/detail/connection.hpp>
#include <userver/storages/postgres/detail/query_parameters.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utest/log_capture_fixture.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;

namespace {

constexpr std::chrono::seconds kTransactionPoolerNetworkTimeout{2};
constexpr std::chrono::milliseconds kTransactionPoolerStatementTimeout{200};

constexpr pg::CommandControl kTransactionPoolerCmdCtl{
    kTransactionPoolerNetworkTimeout,
    kTransactionPoolerStatementTimeout,
};
constexpr pg::CommandControl kTransactionPoolerDefaultCmdCtl{
    std::chrono::seconds{10},
    std::chrono::seconds{10},
};
constexpr pg::CommandControl kTransactionPoolerNoPrepareCmdCtl{
    kTransactionPoolerNetworkTimeout,
    kTransactionPoolerStatementTimeout,
    pg::CommandControl::PreparedStatementsOptionOverride::kDisabled,
};

pg::ConnectionSettings MakeTransactionPoolerSettings(pg::ConnectionSettings settings = kCachePreparedStatements) {
    settings.pooler_mode = pg::PoolerMode::kTransaction;
    settings.statement_log_mode = pg::ConnectionSettings::StatementLogMode::kLog;
    return settings;
}

constexpr std::string_view kSetConfigStatementName{"set_config"};

void ZeroBackendStatementTimeout(const pg::detail::ConnectionPtr& conn) {
    conn->Execute("SELECT set_config('statement_timeout', '0', false)");
}

void ExpectBackendStatementTimeout(const pg::detail::ConnectionPtr& conn, std::string_view expected) {
    pg::ResultSet res{nullptr};
    UEXPECT_NO_THROW(res = conn->Execute("SELECT current_setting('statement_timeout')"));
    EXPECT_EQ(expected, res.AsSingleRow<std::string>());
}

class PostgreTransactionModeConnection : public utest::LogCaptureFixture<PostgreSQLBase> {
protected:
    pg::detail::ConnectionPtr MakeConn(const pg::ConnectionSettings& settings = kCachePreparedStatements) {
        pg::detail::ConnectionPtr conn{nullptr};

        UEXPECT_NO_THROW(
            conn = MakeConnection(GetDsnFromEnv(), GetTaskProcessor(), MakeTransactionPoolerSettings(settings))
        );
        EXPECT_TRUE(conn);
        return conn;
    }

    std::vector<utest::LogRecord> GetTransactionControlLogs() {
        return GetLogCapture().Filter([](const utest::LogRecord& log) {
            const auto statement = log.GetTagOptional(tracing::kDatabaseStatement);
            return statement == "BEGIN" || statement == "COMMIT" || statement == "ROLLBACK";
        });
    }
};

UTEST_F(PostgreTransactionModeConnection, TransactionPoolerSkipsStatementTimeoutOnConnect) {
    const auto conn = MakeConn();

    EXPECT_EQ(std::chrono::milliseconds{0}, conn->GetStatementTimeout());
}

UTEST_F(PostgreTransactionModeConnection, TransactionPoolerAppliesDefaultTimeoutAtBegin) {
    const auto conn = MakeConn();

    const DefaultCommandControlScope scope{kTransactionPoolerCmdCtl};

    UEXPECT_NO_THROW(ZeroBackendStatementTimeout(conn));
    UEXPECT_NO_THROW(conn->Begin({}, {}));

    EXPECT_EQ(kTransactionPoolerStatementTimeout, conn->GetStatementTimeout());
    UEXPECT_NO_THROW(ExpectBackendStatementTimeout(conn, "200ms"));

    UEXPECT_NO_THROW(conn->Rollback());
    UEXPECT_NO_THROW(conn->CancelAndCleanup(utest::kMaxTestWaitTime));
    EXPECT_FALSE(conn->IsBroken());
}

UTEST_F(PostgreTransactionModeConnection, TransactionPoolerBeginCommandControlOverridesDefaultTimeout) {
    const auto conn = MakeConn();

    const DefaultCommandControlScope scope{kTransactionPoolerDefaultCmdCtl};

    UEXPECT_NO_THROW(ZeroBackendStatementTimeout(conn));
    UEXPECT_NO_THROW(conn->Begin({}, {}, kTransactionPoolerCmdCtl));

    EXPECT_EQ(kTransactionPoolerStatementTimeout, conn->GetStatementTimeout());
    UEXPECT_NO_THROW(ExpectBackendStatementTimeout(conn, "200ms"));

    UEXPECT_NO_THROW(conn->Rollback());
    UEXPECT_NO_THROW(conn->CancelAndCleanup(utest::kMaxTestWaitTime));
    EXPECT_FALSE(conn->IsBroken());
}

UTEST_F(PostgreTransactionModeConnection, CancelledInTransactionPoolerByStatementTimeout) {
    const auto conn = MakeConn();

    const DefaultCommandControlScope scope{kTransactionPoolerCmdCtl};

    UEXPECT_NO_THROW(ZeroBackendStatementTimeout(conn));
    UEXPECT_NO_THROW(conn->Begin({}, {}));

    UEXPECT_THROW(conn->Execute("SELECT pg_sleep(1.5)"), pg::QueryCancelled);

    EXPECT_EQ(pg::ConnectionState::kTranError, conn->GetState());
    UEXPECT_NO_THROW(conn->Rollback());
    UEXPECT_NO_THROW(conn->CancelAndCleanup(utest::kMaxTestWaitTime));
    EXPECT_EQ(pg::ConnectionState::kIdle, conn->GetState());
    EXPECT_FALSE(conn->IsBroken());
}

UTEST_F(PostgreTransactionModeConnection, TransactionPoolerStatementCommandControlCancelsInTransaction) {
    const auto conn = MakeConn();

    const DefaultCommandControlScope scope{kTransactionPoolerDefaultCmdCtl};

    UEXPECT_NO_THROW(ZeroBackendStatementTimeout(conn));
    UEXPECT_NO_THROW(conn->Begin({}, {}));

    UEXPECT_THROW(
        conn->Execute(kTransactionPoolerCmdCtl, pg::Query{"SELECT pg_sleep(1.5)"}, pg::ParameterStore{}),
        pg::QueryCancelled
    );

    UEXPECT_NO_THROW(conn->Rollback());
    UEXPECT_NO_THROW(conn->CancelAndCleanup(utest::kMaxTestWaitTime));
    EXPECT_FALSE(conn->IsBroken());
}

UTEST_F(PostgreTransactionModeConnection, TransactionPoolerAutocommitAppliesDefaultTimeout) {
    const auto conn = MakeConn();

    const DefaultCommandControlScope scope{kTransactionPoolerCmdCtl};

    UEXPECT_NO_THROW(ZeroBackendStatementTimeout(conn));

    UEXPECT_THROW(conn->Execute("SELECT pg_sleep(1.5)"), pg::QueryCancelled);
    UEXPECT_NO_THROW(conn->CancelAndCleanup(utest::kMaxTestWaitTime));
    EXPECT_FALSE(conn->IsBroken());
}

UTEST_F(PostgreTransactionModeConnection, TransactionPoolerStatementCommandControlCancelsInAutocommit) {
    const auto conn = MakeConn();

    const DefaultCommandControlScope scope{kTransactionPoolerDefaultCmdCtl};

    UEXPECT_NO_THROW(ZeroBackendStatementTimeout(conn));

    UEXPECT_THROW(
        conn->Execute(kTransactionPoolerCmdCtl, pg::Query{"SELECT pg_sleep(1.5)"}, pg::ParameterStore{}),
        pg::QueryCancelled
    );
    UEXPECT_NO_THROW(conn->CancelAndCleanup(utest::kMaxTestWaitTime));
    EXPECT_FALSE(conn->IsBroken());
}

UTEST_F(PostgreTransactionModeConnection, TransactionPoolerSetsStatementTimeoutOncePerTransaction) {
    const auto conn = MakeConn();

    const DefaultCommandControlScope scope{kTransactionPoolerCmdCtl};

    UEXPECT_NO_THROW(ZeroBackendStatementTimeout(conn));
    GetLogCapture().Clear();

    UEXPECT_NO_THROW(conn->Begin({}, {}));
    EXPECT_EQ(kTransactionPoolerStatementTimeout, conn->GetStatementTimeout());
    UEXPECT_NO_THROW(conn->Execute("SELECT 1"));
    UEXPECT_NO_THROW(conn->Execute("SELECT 2"));
    UEXPECT_NO_THROW(conn->Execute("SELECT current_setting('statement_timeout')"));

    const auto set_config_logs = GetLogCapture().Filter([&](const utest::LogRecord& log) {
        return log.GetTagOptional(tracing::kDatabaseStatementName) == kSetConfigStatementName;
    });
    EXPECT_THAT(set_config_logs, ::testing::SizeIs(1));

    UEXPECT_NO_THROW(conn->Rollback());
    UEXPECT_NO_THROW(conn->CancelAndCleanup(utest::kMaxTestWaitTime));
    EXPECT_TRUE(conn->IsIdle());
}

UTEST_F(PostgreTransactionModeConnection, TransactionPoolerResendsSetConfigWhenCustomTimeoutAppliedLater) {
    const auto conn = MakeConn();

    const DefaultCommandControlScope scope{kTransactionPoolerCmdCtl};

    UEXPECT_NO_THROW(ZeroBackendStatementTimeout(conn));
    GetLogCapture().Clear();

    UEXPECT_NO_THROW(conn->Begin({}, {}));
    EXPECT_EQ(kTransactionPoolerStatementTimeout, conn->GetStatementTimeout());
    UEXPECT_NO_THROW(conn->Execute("SELECT 1"));
    UEXPECT_NO_THROW(conn->Execute(kTransactionPoolerDefaultCmdCtl, pg::Query{"SELECT 2"}, pg::ParameterStore{}));

    const auto set_config_logs = GetLogCapture().Filter([&](const utest::LogRecord& log) {
        return log.GetTagOptional(tracing::kDatabaseStatementName) == kSetConfigStatementName;
    });
    EXPECT_THAT(set_config_logs, ::testing::SizeIs(2));

    EXPECT_EQ(std::chrono::milliseconds{9995}, conn->GetStatementTimeout());

    pg::ResultSet backend_timeout{nullptr};
    UEXPECT_NO_THROW(
        backend_timeout = conn->Execute(
            kTransactionPoolerDefaultCmdCtl,
            pg::Query{"SELECT current_setting('statement_timeout')"},
            pg::ParameterStore{}
        )
    );
    EXPECT_EQ("9995ms", backend_timeout.AsSingleRow<std::string>());

    UEXPECT_NO_THROW(conn->Rollback());
    UEXPECT_NO_THROW(conn->CancelAndCleanup(utest::kMaxTestWaitTime));
    EXPECT_TRUE(conn->IsIdle());
}

UTEST_F(PostgreTransactionModeConnection, DuplicatePreparedStatementDirtyConnectionIsCleanedUp) {
    const pg::Query query{"SELECT 42"};

    std::string statement_name;
    {
        const auto conn = MakeConn();
        UEXPECT_NO_THROW(
            statement_name = conn->PrepareStatement(query, {}, utest::kMaxTestWaitTime).meta_statement_name
        );
    }
    ASSERT_FALSE(statement_name.empty());

    const DefaultCommandControlScope begin_scope{kTransactionPoolerCmdCtl};

    const auto conn = MakeConn();

    UEXPECT_NO_THROW(conn->Execute("PREPARE " + statement_name + " AS SELECT 42", {}, kTransactionPoolerNoPrepareCmdCtl)
    );

    UEXPECT_NO_THROW(conn->Begin({}, {}));
    UEXPECT_THROW(conn->Execute(query), pg::DuplicatePreparedStatement);
    ASSERT_EQ(pg::ConnectionState::kTranError, conn->GetState());

    const DefaultCommandControlScope cleanup_scope{kTransactionPoolerDefaultCmdCtl};

    UEXPECT_NO_THROW(conn->Cleanup(utest::kMaxTestWaitTime));
    EXPECT_EQ(pg::ConnectionState::kIdle, conn->GetState());
    EXPECT_FALSE(conn->IsBroken());

    UEXPECT_NO_THROW(conn->Execute("SELECT 1"));
    EXPECT_FALSE(conn->IsBroken());
}

UTEST_F(PostgreTransactionModeConnection, PipelineTransactionPoolerAutocommitAppliesTimeoutWithoutTransaction) {
    const auto conn = MakeConn();

    const DefaultCommandControlScope scope{kTransactionPoolerCmdCtl};

    UEXPECT_NO_THROW(ZeroBackendStatementTimeout(conn));
    GetLogCapture().Clear();

    pg::ResultSet res{nullptr};
    UEXPECT_NO_THROW(res = conn->Execute("SELECT current_setting('statement_timeout')"));
    EXPECT_EQ("200ms", res.AsSingleRow<std::string>());

    const auto set_config_logs = GetLogCapture().Filter([&](const utest::LogRecord& log) {
        return log.GetTagOptional(tracing::kDatabaseStatementName) == kSetConfigStatementName;
    });
    EXPECT_THAT(set_config_logs, ::testing::SizeIs(1));
    EXPECT_THAT(GetTransactionControlLogs(), ::testing::IsEmpty());

    GetLogCapture().Clear();

    const auto user_cmd_ctl = kTransactionPoolerCmdCtl.WithStatementTimeout(std::chrono::milliseconds{250});
    UEXPECT_NO_THROW(
        res =
            conn->Execute(user_cmd_ctl, pg::Query{"SELECT current_setting('statement_timeout')"}, pg::ParameterStore{})
    );
    EXPECT_EQ("250ms", res.AsSingleRow<std::string>());

    const auto set_config_logs_user = GetLogCapture().Filter([&](const utest::LogRecord& log) {
        return log.GetTagOptional(tracing::kDatabaseStatementName) == kSetConfigStatementName;
    });
    EXPECT_THAT(set_config_logs_user, ::testing::SizeIs(1));
    EXPECT_THAT(GetTransactionControlLogs(), ::testing::IsEmpty());

    UEXPECT_NO_THROW(conn->CancelAndCleanup(utest::kMaxTestWaitTime));
    EXPECT_FALSE(conn->IsBroken());
}

UTEST_F(PostgreTransactionModeConnection, PipelineTransactionPoolerCancelledAutocommitLeavesConnectionUsable) {
    const auto conn = MakeConn();

    const DefaultCommandControlScope scope{kTransactionPoolerCmdCtl};

    UEXPECT_NO_THROW(ZeroBackendStatementTimeout(conn));
    GetLogCapture().Clear();

    UEXPECT_THROW(conn->Execute("SELECT pg_sleep(1.5)"), pg::QueryCancelled);

    EXPECT_EQ(pg::ConnectionState::kIdle, conn->GetState());
    EXPECT_THAT(GetTransactionControlLogs(), ::testing::IsEmpty());

    UEXPECT_NO_THROW(conn->Execute("SELECT 1"));
    EXPECT_FALSE(conn->IsBroken());
}

UTEST_F(PostgreTransactionModeConnection, TransactionPoolerAutocommitRetriesDuplicatePreparedStatement) {
    const pg::Query query{"SELECT 42"};

    std::string statement_name;
    {
        const auto conn = MakeConn();
        UEXPECT_NO_THROW(
            statement_name = conn->PrepareStatement(query, {}, utest::kMaxTestWaitTime).meta_statement_name
        );
    }
    ASSERT_FALSE(statement_name.empty());

    const DefaultCommandControlScope scope{kTransactionPoolerDefaultCmdCtl};

    const auto conn = MakeConn();

    UEXPECT_NO_THROW(conn->Execute("PREPARE " + statement_name + " AS SELECT 42", {}, kTransactionPoolerNoPrepareCmdCtl)
    );

    conn->GetStatsAndReset();

    pg::ResultSet res{nullptr};
    UEXPECT_NO_THROW(res = conn->Execute(query));
    EXPECT_EQ(42, res.AsSingleRow<int>());

    const auto stats = conn->GetStatsAndReset();
    EXPECT_EQ(1, stats.duplicate_prepared_statements);

    EXPECT_EQ(pg::ConnectionState::kIdle, conn->GetState());
    EXPECT_FALSE(conn->IsBroken());
}

UTEST_F(PostgreTransactionModeConnection, DuplicatePreparedStatementInUserTransactionThrows) {
    const pg::Query query{"SELECT 42"};

    std::string statement_name;
    {
        const auto conn = MakeConn();
        UEXPECT_NO_THROW(
            statement_name = conn->PrepareStatement(query, {}, utest::kMaxTestWaitTime).meta_statement_name
        );
    }
    ASSERT_FALSE(statement_name.empty());

    const DefaultCommandControlScope scope{kTransactionPoolerDefaultCmdCtl};

    const auto conn = MakeConn();

    UEXPECT_NO_THROW(conn->Execute("PREPARE " + statement_name + " AS SELECT 42", {}, kTransactionPoolerNoPrepareCmdCtl)
    );

    conn->GetStatsAndReset();

    UEXPECT_NO_THROW(conn->Begin({}, {}));
    UEXPECT_THROW(conn->Execute(query), pg::DuplicatePreparedStatement);
    EXPECT_EQ(pg::ConnectionState::kTranError, conn->GetState());

    UEXPECT_NO_THROW(conn->Rollback());
    EXPECT_EQ(pg::ConnectionState::kIdle, conn->GetState());
    EXPECT_FALSE(conn->IsBroken());

    const auto stats = conn->GetStatsAndReset();
    EXPECT_EQ(1, stats.duplicate_prepared_statements);
}

UTEST_F(PostgreTransactionModeConnection, DuplicatePreparedStatementInUserTransactionWithPipeliningThrows) {
    const pg::Query query{"SELECT 42"};

    std::string statement_name;
    {
        const auto conn = MakeConn();
        UEXPECT_NO_THROW(
            statement_name = conn->PrepareStatement(query, {}, utest::kMaxTestWaitTime).meta_statement_name
        );
    }
    ASSERT_FALSE(statement_name.empty());

    const DefaultCommandControlScope scope{kTransactionPoolerDefaultCmdCtl};

    const auto conn = MakeConn();
    conn->AssertPipelineActive();

    UEXPECT_NO_THROW(conn->Execute("PREPARE " + statement_name + " AS SELECT 42", {}, kTransactionPoolerNoPrepareCmdCtl)
    );

    conn->GetStatsAndReset();

    UEXPECT_NO_THROW(conn->Begin({}, {}));
    UEXPECT_THROW(conn->Execute(query), pg::DuplicatePreparedStatement);
    EXPECT_EQ(pg::ConnectionState::kTranError, conn->GetState());

    UEXPECT_NO_THROW(conn->Rollback());
    EXPECT_EQ(pg::ConnectionState::kIdle, conn->GetState());
    EXPECT_FALSE(conn->IsBroken());

    const auto stats = conn->GetStatsAndReset();
    EXPECT_EQ(1, stats.duplicate_prepared_statements);
}

}  // namespace

USERVER_NAMESPACE_END
