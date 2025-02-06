#include <userver/utest/utest.hpp>

#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <userver/logging/log.hpp>
#include <userver/storages/sqlite.hpp>
#include <userver/storages/sqlite/connection.hpp>
#include <userver/storages/sqlite/execution_result.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/row_types.hpp>
#include <userver/storages/sqlite/tests/utils.hpp>
#include <userver/storages/sqlite/transaction.hpp>
#include <userver/utest/assert_macros.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

// Here we check the high-level operation of transactions; this requires a test
// connection to the database

class SQLiteTransactions : public SQLiteInMemoryInitConnection {};

UTEST_F(SQLiteTransactions, Commit) {
  ConnectionPtr conn;
  UEXPECT_NO_THROW(conn = CreateConnection())
      << "Connect to in-memory database";

  Transaction trx{nullptr, {}};
  UEXPECT_NO_THROW(trx = conn->Begin("test_trx_commit", {}))
      << "Begin default transaction";
  ExecutionResult exec_result;
  UEXPECT_NO_THROW(exec_result =
                       trx.Execute("INSERT INTO test VALUES (1, 'first')")
                           .AsExecutionResult())
      << "Insert row in transaction";
  EXPECT_EQ(1, exec_result.rows_affected);
  EXPECT_EQ(1, exec_result.last_insert_id);
  UEXPECT_NO_THROW(trx.Commit()) << "Commit transaction";
  UEXPECT_THROW(trx.Commit(), sqlite::SQLiteException)
      << "Commit again throw an exception";
  UEXPECT_THROW(trx.Rollback(), sqlite::SQLiteException)
      << "Rollback after commit throw an exception";

  std::string res;
  UEXPECT_NO_THROW(
      res =
          conn->Execute("SELECT value FROM test").AsSingleField<std::string>());
  EXPECT_EQ("first", res);
}

UTEST_F(SQLiteTransactions, Rollback) {
  ConnectionPtr conn;
  UEXPECT_NO_THROW(conn = CreateConnection())
      << "Connect to in-memory database";

  Transaction trx{nullptr, {}};
  UEXPECT_NO_THROW(trx = conn->Begin("test_trx_commit", {}))
      << "Begin default transaction";
  int last_insert_id{};
  UEXPECT_NO_THROW(last_insert_id =
                       trx.Execute("INSERT INTO test VALUES (NULL, 'first')")
                           .AsExecutionResult()
                           .last_insert_id)
      << "Insert row in transaction";
  EXPECT_EQ(1, last_insert_id);
  UEXPECT_NO_THROW(trx.Rollback()) << "Rollback transaction";
  UEXPECT_THROW(trx.Commit(), sqlite::SQLiteException)
      << "Commit again throw an exception";
  UEXPECT_THROW(trx.Rollback(), sqlite::SQLiteException)
      << "Rollback after commit throw an exception";

  std::vector<std::string> res;
  UEXPECT_NO_THROW(res = conn->Execute("SELECT value FROM test")
                             .AsVector<std::string>(kFieldTag));
  EXPECT_TRUE(res.empty());
}

UTEST_F(SQLiteTransactions, AutoRollback) {
  ConnectionPtr conn;
  UEXPECT_NO_THROW(conn = CreateConnection())
      << "Connect to in-memory database";

  // Insert a row and not commit the transaction
  {
    Transaction trx{nullptr, {}};
    UEXPECT_NO_THROW(trx = conn->Begin("test_trx_commit", {}))
        << "Begin default transaction";
    int last_insert_id{};
    UEXPECT_NO_THROW(last_insert_id =
                         trx.Execute("INSERT INTO test VALUES (NULL, 'first')")
                             .AsExecutionResult()
                             .last_insert_id)
        << "Insert row in transaction";
    EXPECT_EQ(1, last_insert_id);
  }

  // Insert a row and rollback the transaction -> auto rollback not throw
  // exception
  {
    Transaction trx{nullptr, {}};
    UEXPECT_NO_THROW(trx = conn->Begin("test_trx_commit", {}))
        << "Begin default transaction";
    int last_insert_id{};
    UEXPECT_NO_THROW(last_insert_id =
                         trx.Execute("INSERT INTO test VALUES (NULL, 'first')")
                             .AsExecutionResult()
                             .last_insert_id)
        << "Insert row in transaction";
    EXPECT_EQ(1, last_insert_id);
    UEXPECT_NO_THROW(trx.Rollback()) << "Rollback transaction";
  }

  // Failure (exception) in transaction is safe
  try {
    Transaction trx{nullptr, {}};
    UEXPECT_NO_THROW(trx = conn->Begin("test_trx_commit", {}))
        << "Begin default transaction";
    int last_insert_id{};
    UEXPECT_NO_THROW(last_insert_id =
                         trx.Execute("INSERT INTO test VALUES (NULL, 'first')")
                             .AsExecutionResult()
                             .last_insert_id)
        << "Insert row in transaction";
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
