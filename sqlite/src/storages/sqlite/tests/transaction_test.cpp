#include <userver/utest/utest.hpp>

#include <gtest/gtest.h>

#include <userver/logging/log.hpp>
#include <userver/utest/assert_macros.hpp>

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
};

}  // namespace

UTEST_F(SQLiteTransactionsTest, Commit) {
    ClientPtr client;
    UEXPECT_NO_THROW(client = CreateClient()) << "Connect to in-memory database";

    Transaction trx{nullptr, {}};
    UEXPECT_NO_THROW(trx = client->Begin(OperationType::kReadWrite, {})) << "Begin default transaction";
    ExecutionResult exec_result;
    UEXPECT_NO_THROW(exec_result = trx.Execute("INSERT INTO test VALUES (1, 'first')").AsExecutionResult())
        << "Insert row in transaction";
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
