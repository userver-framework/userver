#include <userver/utest/utest.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <userver/logging/log.hpp>
#include <userver/utest/death_tests.hpp>

#include <userver/storages/sqlite/impl/connection.hpp>
#include <userver/storages/sqlite/infra/connection_ptr.hpp>
#include <userver/storages/sqlite/tests/utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

class SQLiteSavepoints : public SQLiteInMemoryInitConnection {};

UTEST_F(SQLiteSavepoints, Release) {
  ClientPtr client;
  UEXPECT_NO_THROW(client = CreateClient()) << "Connect to in-memory database";

  Savepoint savepoint{infra::ConnectionPtr{nullptr, nullptr}, {}};
  UEXPECT_NO_THROW(
      savepoint = client->Save(OperationType::kReadWrite, "test_savepoint"))
      << "Begin savepoint";
  ExecutionResult exec_result;
  UEXPECT_NO_THROW(
      exec_result = savepoint.Execute("INSERT INTO test VALUES (1, 'first') ")
                        .AsExecutionResult())
      << "Insert row in savepoint";
  EXPECT_EQ(1, exec_result.rows_affected);
  EXPECT_EQ(1, exec_result.last_insert_id);
  UEXPECT_NO_THROW(savepoint.Release()) << "Release savepoint";
  // After that savepoint has been released it's invalid to use

  std::string res;
  UEXPECT_NO_THROW(
      res = client->Execute(OperationType::kReadOnly, "SELECT value FROM test")
                .AsSingleField<std::string>());
  EXPECT_EQ("first", res);
}

class SQLiteSavepointsDeathTest : public SQLiteSavepoints {};

UTEST_F_DEATH(SQLiteSavepointsDeathTest, UseAfterReleaseDeathTest) {
  ClientPtr client;
  UEXPECT_NO_THROW(client = CreateClient()) << "Connect to in-memory database";

  Savepoint savepoint{infra::ConnectionPtr{nullptr, nullptr}, {}};
  UEXPECT_NO_THROW(
      savepoint = client->Save(OperationType::kReadWrite, "test_savepoint"))
      << "Begin savepoint";
  UEXPECT_NO_THROW(savepoint.Release()) << "Release savepoint";

  // After that savepoint has been released it's invalid to use
  UEXPECT_DEATH(savepoint.Execute("INSERT INTO test VALUES (1, 'first')")
                    .AsExecutionResult(),
                "");
  UEXPECT_DEATH(savepoint.Release(), "");
  UEXPECT_DEATH(savepoint.RollbackTo(), "");
}

UTEST_F(SQLiteSavepoints, RollbackTo) {
  ClientPtr client;
  UEXPECT_NO_THROW(client = CreateClient()) << "Connect to in-memory database";

  Savepoint savepoint{infra::ConnectionPtr{nullptr, nullptr}, {}};
  UEXPECT_NO_THROW(
      savepoint = client->Save(OperationType::kReadWrite, "test_savepoint"))
      << "Begin savepoint";
  ExecutionResult exec_result;
  UEXPECT_NO_THROW(
      exec_result = savepoint.Execute("INSERT INTO test VALUES (1, 'first') ")
                        .AsExecutionResult())
      << "Insert row in savepoint";
  EXPECT_EQ(1, exec_result.rows_affected);
  EXPECT_EQ(1, exec_result.last_insert_id);
  UEXPECT_NO_THROW(savepoint.RollbackTo()) << "Rollback to savepoint";
  UEXPECT_NO_THROW(savepoint.RollbackTo())
      << "Rollback again no throw any exception";
  UEXPECT_NO_THROW(savepoint.Release())
      << "Release after rollback not throw any exception";

  std::vector<std::string> res;
  UEXPECT_NO_THROW(
      res = client->Execute(OperationType::kReadOnly, "SELECT value FROM test")
                .AsVector<std::string>(kFieldTag));
  EXPECT_TRUE(res.empty());
  // After it transaction (savepoint would be end)
}

UTEST_F(SQLiteSavepoints, AutoRollback) {
  ClientPtr client;
  UEXPECT_NO_THROW(client = CreateClient()) << "Connect to in-memory database";

  // Insert a row and not release the savepoint
  {
    Savepoint savepoint{infra::ConnectionPtr{nullptr, nullptr}, {}};
    UEXPECT_NO_THROW(
        savepoint = client->Save(OperationType::kReadWrite, "test_savepoint"))
        << "Begin savepoint";
    int last_insert_id{};
    UEXPECT_NO_THROW(
        last_insert_id =
            savepoint.Execute("INSERT INTO test VALUES (NULL, 'first')")
                .AsExecutionResult()
                .last_insert_id)
        << "Insert row in savepoint";
    EXPECT_EQ(1, last_insert_id);
  }

  // Insert a row and rollback the savepoint -> auto rollback not throw
  // exception
  {
    Savepoint savepoint{infra::ConnectionPtr{nullptr, nullptr}, {}};
    UEXPECT_NO_THROW(
        savepoint = client->Save(OperationType::kReadWrite, "test_savepoint"))
        << "Begin savepoint";
    int last_insert_id{};
    UEXPECT_NO_THROW(
        last_insert_id =
            savepoint.Execute("INSERT INTO test VALUES (NULL, 'first')")
                .AsExecutionResult()
                .last_insert_id)
        << "Insert row in savepoint";
    EXPECT_EQ(1, last_insert_id);
    UEXPECT_NO_THROW(savepoint.RollbackTo()) << "Rollback savepoint";
  }

  // Failure (exception) in savepoint is safe
  try {
    Savepoint savepoint{infra::ConnectionPtr{nullptr, nullptr}, {}};
    UEXPECT_NO_THROW(
        savepoint = client->Save(OperationType::kReadWrite, "test_savepoint"))
        << "Begin default savepoint";
    int last_insert_id{};
    UEXPECT_NO_THROW(
        last_insert_id =
            savepoint.Execute("INSERT INTO test VALUES (NULL, 'first')")
                .AsExecutionResult()
                .last_insert_id)
        << "Insert row in savepoint";
    EXPECT_EQ(1, last_insert_id);
    savepoint.Execute("Incorrect query");
    GTEST_FATAL_FAILURE_("Should never be here");
    UEXPECT_NO_THROW(savepoint.Release()) << "Release savepoint";
  } catch (std::exception& e) {
    LOG_ERROR() << "SQLite exception: " << e.what();
  }
}

UTEST_F(SQLiteSavepoints, MultipleSavepoints) {
  ClientPtr client;
  UEXPECT_NO_THROW(client = CreateClient()) << "Connect to in-memory database";

  // Release both savepoints
  {
    Savepoint savepoint1{infra::ConnectionPtr{nullptr, nullptr}, {}};
    UEXPECT_NO_THROW(
        savepoint1 = client->Save(OperationType::kReadWrite, "test_savepoint1"))
        << "Begin first savepoint";
    UEXPECT_NO_THROW(
        savepoint1.Execute("INSERT INTO test VALUES (NULL, 'first')"))
        << "Insert first row in savepoint";

    Savepoint savepoint2{infra::ConnectionPtr{nullptr, nullptr}, {}};
    UEXPECT_NO_THROW(savepoint2 = savepoint1.Save("test_savepoint2"))
        << "Begin second savepoint";
    UEXPECT_NO_THROW(
        savepoint2.Execute("INSERT INTO test VALUES (NULL, 'second')"))
        << "Insert second row in savepoint";

    UEXPECT_NO_THROW(savepoint1.Release())
        << "Commit all nested transactions (savepoints)";
  }

  // one release, one rollback
  {
    Savepoint savepoint1{infra::ConnectionPtr{nullptr, nullptr}, {}};
    UEXPECT_NO_THROW(
        savepoint1 = client->Save(OperationType::kReadWrite, "test_savepoint1"))
        << "Begin first savepoint";
    UEXPECT_NO_THROW(
        savepoint1.Execute("INSERT INTO test VALUES (NULL, 'first')"))
        << "Insert first row in savepoint";

    Savepoint savepoint2{infra::ConnectionPtr{nullptr, nullptr}, {}};
    UEXPECT_NO_THROW(savepoint2 = savepoint1.Save("test_savepoint2"))
        << "Begin second savepoint";
    UEXPECT_NO_THROW(
        savepoint2.Execute("INSERT INTO test VALUES (NULL, 'second')"))
        << "Insert second row in savepoint";

    UEXPECT_NO_THROW(savepoint2.RollbackTo())
        << "Rollback iternal transaction ";
    UEXPECT_NO_THROW(savepoint1.Release()) << "Commit iternal transaction ";
  }

  std::vector<std::string> res;
  UEXPECT_NO_THROW(
      res = client->Execute(OperationType::kReadOnly, "SELECT value FROM test")
                .AsVector<std::string>(kFieldTag));
  EXPECT_EQ(3, res.size());
}

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
