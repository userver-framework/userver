#include <userver/utest/utest.hpp>

#include <gtest/gtest.h>

#include <userver/concurrent/background_task_storage.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/storages/sqlite.hpp>
#include <userver/storages/sqlite/connection.hpp>
#include <userver/storages/sqlite/tests/utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

namespace {

struct Row final {
  int id{};
  std::string value;

  bool operator==(const Row& other) const {
    return std::tie(id, value) == std::tie(other.id, other.value);
  }
};

using RowTuple = std::tuple<int, std::string>;

constexpr std::string_view kSelectAllRows = "SELECT * FROM test";
constexpr std::string_view kSelectOneRow = "SELECT * FROM test WHERE id=1";
constexpr std::string_view kSelectNullRow = "SELECT * FROM test WHERE id=NULL";

class SQLiteResultSet : public SQLiteInMemoryInitConnection {
 public:
  void Init(ConnectionPtr connection) {
    connection->Execute("INSERT INTO test VALUES (1, 'first')");
    connection->Execute("INSERT INTO test VALUES (2, 'second')");
  }
};

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

UTEST_F(SQLiteResultSet, Execute) {
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
}

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
