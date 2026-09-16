#include <gmock/gmock.h>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/tests/util_pgtest.hpp>
#include <userver/storages/postgres/detail/non_transaction.hpp>
#include <userver/storages/postgres/portal.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utest/log_capture_fixture.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace pg = storages::postgres;

pg::ConnectionSettings MakeSettings() {
    pg::ConnectionSettings settings;
    settings.prepared_statements = pg::ConnectionSettings::kCachePreparedStatements;
    settings.statement_log_mode = pg::ConnectionSettings::StatementLogMode::kLog;
    return settings;
}

const pg::Query::Name kQueryName{R"~(select_best_names)~"};
const std::string kStatement{R"~(SELECT * FROM (VALUES ('Nikita'), ('Christina')) AS names)~"};
const pg::Query kTestQuery(kStatement, kQueryName);
const pg::Query kUnnamedQuery(kStatement);

class LoggableConnection : public utest::LogCaptureFixture<PostgreConnectionBaseFixture> {
protected:
    void CheckNamedLogCapture() {
        const utest::LogCaptureLogger& logger = GetLogCapture();

        const std::vector<utest::LogRecord> logs_with_query_name = logger.Filter([&](const utest::LogRecord& log) {
            return log.GetTagOptional(tracing::kDatabaseStatementName) == kQueryName.GetUnderlying();
        });
        ASSERT_THAT(logs_with_query_name, ::testing::SizeIs(::testing::Gt(0)));

        for (const utest::LogRecord& log : logs_with_query_name) {
            EXPECT_EQ(log.GetTagOptional(tracing::kDatabaseStatement), std::nullopt);
        }
    }

    void CheckUnnamedLogCapture() {
        const utest::LogCaptureLogger& logger = GetLogCapture();

        const std::vector<utest::LogRecord> logs_with_statement = logger.Filter([&](const utest::LogRecord& log) {
            return log.GetTagOptional(tracing::kDatabaseStatement) == kStatement;
        });
        ASSERT_THAT(logs_with_statement, ::testing::SizeIs(::testing::Gt(0)));

        for (const utest::LogRecord& log : logs_with_statement) {
            EXPECT_EQ(log.GetTagOptional(tracing::kDatabaseStatementName), std::nullopt);
        }
    }
};

UTEST_F(LoggableConnection, PortalStatementLogs) {
    auto conn = MakeConn(MakeSettings());
    CheckConnection(conn);

    pg::Transaction trx{
        std::move(conn),
        pg::TransactionOptions{},
    };

    pg::Portal portal{nullptr, "", {}};

    UEXPECT_NO_THROW(portal = trx.MakePortal(kTestQuery));
    // Fetch all
    portal.Fetch(0);
    EXPECT_TRUE(portal.Done());
    UEXPECT_NO_THROW(trx.Commit());

    CheckNamedLogCapture();
}

UTEST_F(LoggableConnection, TrxStatementLogs) {
    auto conn = MakeConn(MakeSettings());
    CheckConnection(conn);

    pg::Transaction trx{
        std::move(conn),
        pg::TransactionOptions{},
    };

    UEXPECT_NO_THROW(trx.Execute(kTestQuery));
    UEXPECT_NO_THROW(trx.Commit());

    CheckNamedLogCapture();
}

UTEST_F(LoggableConnection, NonTrxStatementLogs) {
    auto conn = MakeConn(MakeSettings());
    CheckConnection(conn);

    pg::detail::NonTransaction ntrx(std::move(conn));

    UEXPECT_NO_THROW(ntrx.Execute(kTestQuery));

    CheckNamedLogCapture();
}

UTEST_F(LoggableConnection, NonTrxParameterStoreStatementLogs) {
    auto conn = MakeConn(MakeSettings());
    CheckConnection(conn);

    pg::detail::NonTransaction ntrx(std::move(conn));

    UEXPECT_NO_THROW(ntrx.Execute(kTestQuery, pg::ParameterStore{}));

    CheckNamedLogCapture();
}

UTEST_F(LoggableConnection, UnnamedStatementLogs) {
    auto conn = MakeConn(MakeSettings());
    CheckConnection(conn);

    pg::detail::NonTransaction ntrx(std::move(conn));

    UEXPECT_NO_THROW(ntrx.Execute(kUnnamedQuery));

    CheckUnnamedLogCapture();
}

}  // namespace

USERVER_NAMESPACE_END
