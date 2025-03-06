#include <userver/utest/utest.hpp>

#include <tuple>
#include <vector>

#include <gtest/gtest.h>

#include <userver/concurrent/background_task_storage.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/storages/sqlite.hpp>
#include <userver/storages/sqlite/client.hpp>
#include <userver/storages/sqlite/exceptions.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/tests/utils.hpp>
#include <userver/utest/assert_macros.hpp>

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
  settings::SQLiteSettings settings;
  settings.db_name = GetTestDbPath("test.db");
  settings.create_file = false;

  UEXPECT_THROW(CreateClient(settings), sqlite::SQLiteException)
      << "Connecting to a non-existent database";
}

UTEST_F(SQLiteCustomConnection, CheckRO) {
  // Try to open a non-existing database
  settings::SQLiteSettings settings;
  settings.db_name = GetTestDbPath("test.db");
  settings.create_file = true;

  ClientPtr client;
  UEXPECT_NO_THROW(client = CreateClient(settings));
  UEXPECT_NO_THROW(client->Execute(
      "CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)"));
  UEXPECT_NO_THROW(client->Execute("INSERT INTO test VALUES (1, 'first') "));
  UEXPECT_NO_THROW(client->Execute("INSERT INTO test VALUES (2, 'second')"));
  UEXPECT_THROW(client->Execute(settings::CommandControl::ReadOnly(),
                                "INSERT INTO test VALUES (3, 'third')"),
                SQLiteException);
  UEXPECT_THROW(
      (client
           ->Execute(settings::CommandControl::ReadOnly(),
                     "INSERT INTO test VALUES (3, 'third') RETURNING *")
           .AsVector<std::tuple<int, std::string>>()),
      SQLiteException);
  UEXPECT_NO_THROW(
      (client->Execute("INSERT INTO test VALUES (3, 'third') RETURNING *")
           .AsVector<std::tuple<int, std::string>>()));
  UEXPECT_NO_THROW((client->Execute("SELECT * FROM test")
                        .AsVector<std::tuple<int, std::string>>()));
  UEXPECT_NO_THROW(
      (client
           ->Execute(settings::CommandControl::ReadOnly(), "SELECT * FROM test")
           .AsVector<std::tuple<int, std::string>>()));
}

UTEST_F(SQLiteCustomConnection, CreateOpen) {
  // Try to open a non-existing database
  settings::SQLiteSettings settings;
  settings.db_name = GetTestDbPath("test.db");
  settings.create_file = true;

  UEXPECT_NO_THROW(CreateClient(settings))
      << "Connect to a non-existent database, but the file will be created "
         "automatically";

  // Try to open existing database
  settings.create_file = false;
  UEXPECT_NO_THROW(CreateClient(settings))
      << "Connect to a existent database, but the file will be created "
         "automatically";
}

UTEST_F(SQLiteCustomConnection, InMemory) {
  // Try to open in-memory database
  settings::SQLiteSettings settings;
  settings.db_name = ":memory:";

  UEXPECT_NO_THROW(CreateClient(settings)) << "Connect to in-memory database";
}

UTEST_F(SQLiteResultSet, SuccessExecute) {
  auto client = CreateClient();
  Init(client);

  // Get result as vector of tuples
  {
    std::vector<RowTuple> actual;
    UEXPECT_NO_THROW(actual =
                         client
                             ->Execute(settings::CommandControl::ReadOnly(),
                                       kSelectAllRows.data())
                             .AsVector<RowTuple>());

    EXPECT_EQ(actual.size(), 2);
    EXPECT_EQ(actual[0], std::make_tuple(1, "first"));
    EXPECT_EQ(actual[1], std::make_tuple(2, "second"));
  }

  // Get result as struct
  {
    Row actual;
    UEXPECT_NO_THROW(actual =
                         client
                             ->Execute(settings::CommandControl::ReadOnly(),
                                       kSelectOneRow.data())
                             .AsSingleRow<Row>());

    EXPECT_EQ(actual, (Row{1, "first"}));
  }

  // Get empty result as vector of rows
  {
    std::vector<Row> actual;
    UEXPECT_NO_THROW(actual =
                         client
                             ->Execute(settings::CommandControl::ReadOnly(),
                                       kSelectNullRow.data())
                             .AsVector<Row>());

    EXPECT_TRUE(actual.empty());
  }

  // Get result as a single field
  {
    std::string actual;
    UEXPECT_NO_THROW(actual =
                         client
                             ->Execute(settings::CommandControl::ReadOnly(),
                                       kSelectOneField.data())
                             .AsSingleField<std::string>());

    EXPECT_EQ(actual, "first");
  }

  // Select with unexpected types
  // (https://www.sqlite.org/lang_expr.html#castexpr)
  // also have STRICT mode which fix it
  {
    using UnexpectedRowTuple = std::tuple<std::string, int>;
    std::vector<UnexpectedRowTuple> actual;
    UEXPECT_NO_THROW(actual =
                         client
                             ->Execute(settings::CommandControl::ReadOnly(),
                                       kSelectAllRows.data())
                             .AsVector<UnexpectedRowTuple>());
    EXPECT_EQ(actual[0], std::make_tuple("1", 0));
    EXPECT_EQ(actual[1], std::make_tuple("2", 0));
  }
}

UTEST_F(SQLiteResultSet, FailureExecute) {
  auto client = CreateClient();
  Init(client);

  // Throw exception if try to get set of row as vector of fields
  {
    UEXPECT_THROW(client
                      ->Execute(settings::CommandControl::ReadOnly(),
                                kSelectAllRows.data())
                      .AsVector<std::string>(kFieldTag),
                  SQLiteException);
  }

  // Throw exception if try to get single row from empty result
  {
    UEXPECT_THROW(client
                      ->Execute(settings::CommandControl::ReadOnly(),
                                kSelectNullRow.data())
                      .AsSingleRow<Row>(),
                  SQLiteException);
  }

  // Throw exception if result is empty
  {
    UEXPECT_THROW(client
                      ->Execute(settings::CommandControl::ReadOnly(),
                                kSelectNullField.data())
                      .AsSingleField<std::string>(),
                  SQLiteException);
  }

  // Select with unexpected fields
  {
    std::vector<RowTuple> actual;
    UEXPECT_THROW(actual = client
                               ->Execute(settings::CommandControl::ReadOnly(),
                                         kUnexpectedFieldsSelect.data())
                               .AsVector<RowTuple>(),
                  SQLiteException);
  }

  // Insert unexpected fields (datatype mismatch)
  {
    UEXPECT_THROW(client->Execute(settings::CommandControl::ReadOnly(),
                                  kDatatypeMismatchInsert.data()),
                  SQLiteException);
  }
}

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
