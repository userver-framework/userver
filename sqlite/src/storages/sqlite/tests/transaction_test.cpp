#include <userver/utest/utest.hpp>

#include <gtest/gtest.h>

#include <userver/logging/log.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utils/trx_tracker.hpp>

#include <userver/storages/sqlite/exceptions.hpp>
#include <userver/storages/sqlite/impl/connection.hpp>
#include <userver/storages/sqlite/infra/connection_ptr.hpp>

#include <storages/sqlite/tests/utils_test.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

// Here we check the high-level operation of transactions;
// this requires a test connection to the database

namespace {

class SQLiteTransactionsTest : public SQLiteCompositeFixture<SQLiteInMemoryConnection> {
    void PreInitialize(const ClientPtr& client) final {
        UEXPECT_NO_THROW(
            client->Execute(OperationType::kReadWrite, "CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)")
        );
    }

    void CleanUp(const ClientPtr& client) final {
        UEXPECT_NO_THROW(client->Execute(OperationType::kReadWrite, "DROP TABLE test"));
    }
};

}  // namespace

UTEST_F(SQLiteTransactionsTest, Commit) {
    ClientPtr client;
    UEXPECT_NO_THROW(client = CreateClient()) << "Connect to in-memory database";

    Transaction trx{nullptr, {}};
    UEXPECT_NO_THROW(trx = client->Begin(OperationType::kReadWrite, {})) << "Begin default transaction";
    ExecutionResult exec_result;
    UEXPECT_NO_THROW(exec_result = trx.Execute("INSERT INTO test VALUES (1, 'first')").AsExecutionResult())
        << "Insert "
           "row in "
           "transac"
           "tion";
    EXPECT_EQ(1, exec_result.rows_affected);
    EXPECT_EQ(1, exec_result.last_insert_id);
    UEXPECT_NO_THROW(trx.Commit()) << "Commit transaction";

    std::string res;
    UEXPECT_NO_THROW(
        res = client->Execute(OperationType::kReadOnly, "SELECT value FROM test").AsSingleField<std::string>()
    );
    EXPECT_EQ("first", res);
}

UTEST_F(SQLiteTransactionsTest, Rollback) {
    ClientPtr client;
    UEXPECT_NO_THROW(client = CreateClient()) << "Connect to in-memory database";

    Transaction trx{nullptr, {}};
    UEXPECT_NO_THROW(trx = client->Begin(OperationType::kReadWrite, {})) << "Begin default transaction";
    int last_insert_id{};
    UEXPECT_NO_THROW(
        last_insert_id = trx.Execute("INSERT INTO test VALUES (NULL, 'first')").AsExecutionResult().last_insert_id
    ) << "Insert row in transaction";
    EXPECT_EQ(1, last_insert_id);
    UEXPECT_NO_THROW(trx.Rollback()) << "Rollback transaction";

    std::vector<std::string> res;
    UEXPECT_NO_THROW(
        res = client->Execute(OperationType::kReadOnly, "SELECT value FROM test").AsVector<std::string>(kFieldTag)
    );
    EXPECT_TRUE(res.empty());
}

using SQLiteFileTransactionsTest = SQLiteCompositeFixture<SQLiteCustomConnection>;

UTEST_F(SQLiteFileTransactionsTest, FailedCommitRollsBackBeforeReusingWriter) {
    settings::SQLiteSettings settings;
    settings.db_path = GetTestDbPath("test.db");
    const auto client = CreateClient(settings);
    client->Execute(
        OperationType::kReadWrite,
        "CREATE TEMP TABLE nodes (id INTEGER PRIMARY KEY, "
        "parent INTEGER REFERENCES nodes(id) DEFERRABLE INITIALLY DEFERRED)"
    );

    const utils::trx_tracker::impl::GlobalEnabler enable_tracker;
    utils::trx_tracker::ResetStatistics();

    auto trx = client->Begin(OperationType::kReadWrite, {});
    trx.Execute("INSERT INTO nodes VALUES (1, 2)");
    UEXPECT_THROW(trx.Commit(), SQLiteException);

    utils::trx_tracker::CheckNoTransactions();
    EXPECT_EQ(utils::trx_tracker::GetStatistics().triggers, 0);

    auto next = client->Begin(OperationType::kReadWrite, {});
    EXPECT_EQ(0, next.Execute("SELECT COUNT(*) FROM nodes").AsSingleField<int>());
    next.Execute("INSERT INTO nodes VALUES (1, NULL)");
    next.Commit();
}

UTEST_F(SQLiteTransactionsTest, FailedRollbackDiscardsWriter) {
    const auto client = CreateClient();
    client->Execute(OperationType::kReadWrite, "CREATE TEMP TABLE writer_marker (id INTEGER PRIMARY KEY)");

    auto trx = client->Begin(OperationType::kReadWrite, {});
    trx.Execute("INSERT INTO writer_marker VALUES (1)");
    UEXPECT_THROW(trx.Execute("INSERT OR ROLLBACK INTO writer_marker VALUES (1)"), SQLiteException);
    UEXPECT_THROW(trx.Rollback(), SQLiteException);

    auto next = client->Begin(OperationType::kReadWrite, {});
    const auto marker_count =
        next.Execute("SELECT COUNT(*) FROM sqlite_temp_master WHERE name = 'writer_marker'").AsSingleField<int>();
    EXPECT_EQ(0, marker_count);
    next.Commit();
}

class SQLiteTransactionDeathTest : public SQLiteTransactionsTest {};

UTEST_F_DEATH(SQLiteTransactionDeathTest, UseAfterReleaseDeathTest) {
    ClientPtr client;
    UEXPECT_NO_THROW(client = CreateClient()) << "Connect to in-memory database";

    // Use trx after commit would be abort
    {
        Transaction trx{nullptr, {}};
        UEXPECT_NO_THROW(trx = client->Begin(OperationType::kReadWrite, {})) << "Begin default transaction";
        UEXPECT_NO_THROW(trx.Commit()) << "Commit transaction";
        EXPECT_UINVARIANT_FAILURE(trx.Execute("INSERT INTO test VALUES (1, 'first')").AsExecutionResult());
        EXPECT_UINVARIANT_FAILURE(trx.Commit());
        EXPECT_UINVARIANT_FAILURE(trx.Rollback());
    }

    // Use trx after rollback would be abort
    {
        Transaction trx{nullptr, {}};
        UEXPECT_NO_THROW(trx = client->Begin(OperationType::kReadWrite, {})) << "Begin default transaction";
        UEXPECT_NO_THROW(trx.Rollback()) << "Rollback transaction";
        EXPECT_UINVARIANT_FAILURE(trx.Execute("INSERT INTO test VALUES (1, 'first')").AsExecutionResult());
        EXPECT_UINVARIANT_FAILURE(trx.Commit());
        EXPECT_UINVARIANT_FAILURE(trx.Rollback());
    }
}

UTEST_F(SQLiteTransactionsTest, AutoRollback) {
    ClientPtr client;
    UEXPECT_NO_THROW(client = CreateClient()) << "Connect to in-memory database";

    // Insert a row and not commit the transaction
    {
        Transaction trx{nullptr, {}};
        UEXPECT_NO_THROW(trx = client->Begin(OperationType::kReadWrite, {})) << "Begin default transaction";
        int last_insert_id{};
        UEXPECT_NO_THROW(
            last_insert_id = trx.Execute("INSERT INTO test VALUES (NULL, 'first')").AsExecutionResult().last_insert_id
        ) << "Insert row in transaction";
        EXPECT_EQ(1, last_insert_id);
    }

    // Insert a row and rollback the transaction -> auto rollback not throw
    // exception
    {
        Transaction trx{nullptr, {}};
        UEXPECT_NO_THROW(trx = client->Begin(OperationType::kReadWrite, {})) << "Begin default transaction";
        int last_insert_id{};
        UEXPECT_NO_THROW(
            last_insert_id = trx.Execute("INSERT INTO test VALUES (NULL, 'first')").AsExecutionResult().last_insert_id
        ) << "Insert row in transaction";
        EXPECT_EQ(1, last_insert_id);
        UEXPECT_NO_THROW(trx.Rollback()) << "Rollback transaction";
    }

    // Failure (exception) in transaction is safe
    try {
        Transaction trx{nullptr, {}};
        UEXPECT_NO_THROW(trx = client->Begin(OperationType::kReadWrite, {})) << "Begin default transaction";
        int last_insert_id{};
        UEXPECT_NO_THROW(
            last_insert_id = trx.Execute("INSERT INTO test VALUES (NULL, 'first')").AsExecutionResult().last_insert_id
        ) << "Insert row in transaction";
        EXPECT_EQ(1, last_insert_id);
        trx.Execute("Incorrect query");
        GTEST_FATAL_FAILURE_("Should never be here");
        UEXPECT_NO_THROW(trx.Commit()) << "Commit transaction";
    } catch (std::exception& e) {
        LOG_ERROR() << "SQLite exception: " << e.what();
    }
}

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
