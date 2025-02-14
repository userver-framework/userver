#include <tuple>
#include <userver/utest/utest.hpp>

#include <gtest/gtest.h>

#include <userver/concurrent/background_task_storage.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/storages/sqlite.hpp>
#include <userver/storages/sqlite/connection.hpp>
#include <userver/storages/sqlite/tests/utils.hpp>
#include <vector>
#include "userver/storages/sqlite/exceptions.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

namespace {

constexpr std::string_view kSelectAllRows = "SELECT * FROM test";
constexpr std::string_view kSelectOneRow = "SELECT * FROM test WHERE id=1";
constexpr std::string_view kSelectNullRow = "SELECT * FROM test WHERE id=NULL";
constexpr std::string_view kSelectOneField =
    "SELECT value FROM test WHERE id=1";
constexpr std::string_view kUnexpectedFieldsSelect =
    "SELECT unexpected_field FROM test";
constexpr std::string_view kSelectNullField =
    "SELECT value FROM test WHERE id=NULL";
constexpr std::string_view kDatatypeMismatchInsert =
    "INSERT INTO test VALUES ('third', 3)";
}  // namespace

UTEST_F(SQLiteCustomConnection, NonExistent) {
  // Try to open a non-existing database
  sqlite::SQLiteSettings settings;
  settings.db_name = GetTestDbPath("test.db");
  settings.create_file = false;

  UEXPECT_THROW(CreateConnection(settings), sqlite::SQLiteException)
      << "Connecting to a non-existent database";
}

UTEST_F(SQLiteCustomConnection, CreateOpen) {
  // Try to open a non-existing database
  sqlite::SQLiteSettings settings;
  settings.db_name = GetTestDbPath("test.db");
  settings.create_file = true;

  UEXPECT_NO_THROW(CreateConnection(settings))
      << "Connect to a non-existent database, but the file will be created "
         "automatically";

  // Try to open existing database
  settings.create_file = false;
  UEXPECT_NO_THROW(CreateConnection(settings))
      << "Connect to a existent database, but the file will be created "
         "automatically";
}

UTEST_F(SQLiteCustomConnection, InMemory) {
  // Try to open in-memory database
  sqlite::SQLiteSettings settings;
  settings.db_name = ":memory:";

  UEXPECT_NO_THROW(CreateConnection(settings))
      << "Connect to in-memory database";
}

UTEST_F(SQLiteResultSet, SuccessExecute) {
  ConnectionPtr conn = CreateConnection();
  Init(conn);

  // Get result as vector of tuples
  {
    std::vector<RowTuple> actual;
    UEXPECT_NO_THROW(
        actual = conn->Execute(kSelectAllRows.data()).AsVector<RowTuple>());

    EXPECT_EQ(actual.size(), 2);
    EXPECT_EQ(actual[0], std::make_tuple(1, "first"));
    EXPECT_EQ(actual[1], std::make_tuple(2, "second"));
  }

  // Get result as struct
  {
    Row actual;
    UEXPECT_NO_THROW(
        actual = conn->Execute(kSelectOneRow.data()).AsSingleRow<Row>());

    EXPECT_EQ(actual, (Row{1, "first"}));
  }

  // Get empty result as vector of rows
  {
    std::vector<Row> actual;
    UEXPECT_NO_THROW(actual =
                         conn->Execute(kSelectNullRow.data()).AsVector<Row>());

    EXPECT_TRUE(actual.empty());
  }

  // Get result as a single field
  {
    std::string actual;
    UEXPECT_NO_THROW(
        actual =
            conn->Execute(kSelectOneField.data()).AsSingleField<std::string>());

    EXPECT_EQ(actual, "first");
  }

  // Select with unexpected types
  // (https://www.sqlite.org/lang_expr.html#castexpr)
  // also have STRICT mode which fix it
  {
    using UnexpectedRowTuple = std::tuple<std::string, int>;
    std::vector<UnexpectedRowTuple> actual;
    UEXPECT_NO_THROW(actual = conn->Execute(kSelectAllRows.data())
                                  .AsVector<UnexpectedRowTuple>());
    EXPECT_EQ(actual[0], std::make_tuple("1", 0));
    EXPECT_EQ(actual[1], std::make_tuple("2", 0));
  }
}

UTEST_F(SQLiteResultSet, FailureExecute) {
  ConnectionPtr conn = CreateConnection();
  Init(conn);

  // Throw exception if try to get set of row as vector of fields
  {
    UEXPECT_THROW(
        conn->Execute(kSelectAllRows.data()).AsVector<std::string>(kFieldTag),
        SQLiteException);
  }

  // Throw exception if try to get single row from empty result
  {
    UEXPECT_THROW(conn->Execute(kSelectNullRow.data()).AsSingleRow<Row>(),
                  SQLiteException);
  }

  // Throw exception if result is empty
  {
    UEXPECT_THROW(
        conn->Execute(kSelectNullField.data()).AsSingleField<std::string>(),
        SQLiteException);
  }

  // Select with unexpected fields
  {
    std::vector<RowTuple> actual;
    UEXPECT_THROW(
        actual =
            conn->Execute(kUnexpectedFieldsSelect.data()).AsVector<RowTuple>(),
        SQLiteException);
  }

  // Insert unexpected fields (datatype mismatch)
  {
    UEXPECT_THROW(conn->Execute(kDatatypeMismatchInsert.data()),
                  SQLiteException);
  }
}

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
