#include <userver/utest/utest.hpp>

#include <string_view>
#include <tuple>
#include <vector>

#include <fmt/core.h>
#include <gtest/gtest.h>

#include <userver/engine/async.hpp>
#include <userver/engine/get_all.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utest/assert_macros.hpp>

#include <userver/storages/sqlite/tests/utils.hpp>

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
  settings::SQLiteSettings settings;
  settings.db_name = ":memory:";

  UEXPECT_NO_THROW(CreateClient(settings)) << "Connect to in-memory database";
}

UTEST_F(SQLiteCustomConnection, ReadWrite) {
  settings::SQLiteSettings settings;
  settings.db_name = GetTestDbPath("test.db");
  settings.create_file = true;

  ClientPtr client;
  UEXPECT_NO_THROW(client = CreateClient(settings));
  UEXPECT_NO_THROW(client->Execute(
      storages::sqlite::OperationType::kReadWrite,
      "CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)"));
  UEXPECT_NO_THROW(client->Execute(storages::sqlite::OperationType::kReadWrite,
                                   "INSERT INTO test VALUES (1, 'first') "));
  UEXPECT_NO_THROW(client->Execute(storages::sqlite::OperationType::kReadWrite,
                                   "INSERT INTO test VALUES (2, 'second')"));
  UEXPECT_THROW(client->Execute(storages::sqlite::OperationType::kReadOnly,
                                "INSERT INTO test VALUES (3, 'third')"),
                SQLiteException);
  UEXPECT_THROW(
      (client
           ->Execute(storages::sqlite::OperationType::kReadOnly,
                     "INSERT INTO test VALUES (3, 'third') RETURNING *")
           .AsVector<RowTuple>()),
      SQLiteException);
  UEXPECT_NO_THROW(
      (client
           ->Execute(storages::sqlite::OperationType::kReadWrite,
                     "INSERT INTO test VALUES (3, 'third') RETURNING *")
           .AsVector<RowTuple>()));
  UEXPECT_NO_THROW((client
                        ->Execute(storages::sqlite::OperationType::kReadOnly,
                                  "SELECT * FROM test")
                        .AsVector<RowTuple>()));
  UEXPECT_NO_THROW((client
                        ->Execute(storages::sqlite::OperationType::kReadOnly,
                                  "SELECT * FROM test")
                        .AsVector<RowTuple>()));
}

UTEST_P_MT(SQLiteJournalsTest, ReadWriteConcurent, 10) {
  settings::SQLiteSettings settings;
  settings.db_name = GetTestDbPath("test.db");
  settings.create_file = true;
  settings.journal_mode = settings::SQLiteSettings::JournalMode::kWal;
  constexpr size_t kReadTaskCount = 10;
  constexpr size_t kWriteTaskCount = 10;
  constexpr size_t kReadWriteTaskCount = 10;
  constexpr size_t kTotalTasksCount =
      kReadTaskCount + kWriteTaskCount + kReadWriteTaskCount;

  ClientPtr client;
  UEXPECT_NO_THROW(client = CreateClient(settings)) << "Try to create client";
  UEXPECT_NO_THROW(client->Execute(
      storages::sqlite::OperationType::kReadWrite,
      "CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)"));

  std::vector<engine::TaskWithResult<void>> tasks;
  tasks.reserve(kTotalTasksCount);

  for (size_t i = 0; i < kWriteTaskCount; ++i) {
    tasks.push_back(engine::AsyncNoSpan([&client, i]() {
      UEXPECT_NO_THROW(
          client->Execute(storages::sqlite::OperationType::kReadWrite,
                          "INSERT INTO test VALUES (NULL, 'data')"))
          << fmt::format("Try to insert: ({0}, data_{0})", i);
    }));
  }
  for (size_t i = 0; i < kReadTaskCount; ++i) {
    tasks.push_back(engine::AsyncNoSpan([&client]() {
      UEXPECT_NO_THROW(
          (client
               ->Execute(storages::sqlite::OperationType::kReadOnly,
                         "SELECT * FROM test")
               .AsVector<RowTuple>()))
          << "Try to select all";
    }));
  }
  for (size_t i = kWriteTaskCount; i < kWriteTaskCount + kReadWriteTaskCount;
       ++i) {
    tasks.push_back(engine::AsyncNoSpan([&client, i]() {
      UEXPECT_NO_THROW(
          (client
               ->Execute(storages::sqlite::OperationType::kReadWrite,
                         "INSERT INTO test VALUES (NULL, 'data') RETURNING *")
               .AsVector<RowTuple>()))
          << fmt::format("Try to insert with returning: ({0}, data_{0})", i);
    }));
  }

  engine::GetAll(tasks);
}

INSTANTIATE_UTEST_SUITE_P(
    SQLiteCustomConnection, SQLiteJournalsTest,
    ::testing::Values(settings::SQLiteSettings::JournalMode::kWal,
                      settings::SQLiteSettings::JournalMode::kDelete,
                      settings::SQLiteSettings::JournalMode::kTruncate,
                      settings::SQLiteSettings::JournalMode::kPersist,
                      settings::SQLiteSettings::JournalMode::kMemory,
                      settings::SQLiteSettings::JournalMode::kOff),
    TestParamNameJournalMode);

UTEST_F(SQLiteCustomConnection, ReadOnly) {
  {
    settings::SQLiteSettings settings;
    settings.db_name = GetTestDbPath("test.db");
    settings.create_file = true;
    ClientPtr client;
    UEXPECT_NO_THROW(client = CreateClient(settings))
        << "Connect to a non-existent database, but the file will be "
           "created "
           "automatically";
    UEXPECT_NO_THROW(client->Execute(
        storages::sqlite::OperationType::kReadWrite,
        "CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)"));
    UEXPECT_NO_THROW(
        client->Execute(storages::sqlite::OperationType::kReadWrite,
                        "INSERT INTO test VALUES (1, 'first')"));
    UEXPECT_NO_THROW(
        client->Execute(storages::sqlite::OperationType::kReadWrite,
                        "INSERT INTO test VALUES (2, 'second')"));
  }

  settings::SQLiteSettings settings;
  settings.db_name = GetTestDbPath("test.db");
  settings.create_file = false;
  settings.read_mode = settings::SQLiteSettings::ReadMode::kReadOnly;

  ClientPtr client;
  UEXPECT_NO_THROW(client = CreateClient(settings));
  UEXPECT_THROW(
      (client
           ->Execute(storages::sqlite::OperationType::kReadOnly,
                     "INSERT INTO test VALUES (3, 'third') RETURNING *")
           .AsVector<RowTuple>()),
      SQLiteException);
  UEXPECT_THROW(
      (client
           ->Execute(storages::sqlite::OperationType::kReadWrite,
                     "INSERT INTO test VALUES (3, 'third') RETURNING *")
           .AsVector<RowTuple>()),
      SQLiteException);
  UEXPECT_NO_THROW((client
                        ->Execute(storages::sqlite::OperationType::kReadOnly,
                                  "SELECT * FROM test")
                        .AsVector<RowTuple>()));
  UEXPECT_NO_THROW((client
                        ->Execute(storages::sqlite::OperationType::kReadWrite,
                                  "SELECT * FROM test")
                        .AsVector<RowTuple>()));
}

UTEST_F(SQLiteResultSet, SuccessExecute) {
  auto client = CreateClient();
  Init(client);

  // Get result as vector of tuples
  {
    std::vector<RowTuple> actual;
    UEXPECT_NO_THROW(
        actual = client
                     ->Execute(storages::sqlite::OperationType::kReadOnly,
                               kSelectAllRows.data())
                     .AsVector<RowTuple>());

    EXPECT_EQ(actual.size(), 2);
    EXPECT_EQ(actual[0], std::make_tuple(1, "first"));
    EXPECT_EQ(actual[1], std::make_tuple(2, "second"));
  }

  // Get result as struct
  {
    Row actual;
    UEXPECT_NO_THROW(
        actual = client
                     ->Execute(storages::sqlite::OperationType::kReadOnly,
                               kSelectOneRow.data())
                     .AsSingleRow<Row>());

    EXPECT_EQ(actual, (Row{1, "first"}));
  }

  // Get empty result as vector of rows
  {
    std::vector<Row> actual;
    UEXPECT_NO_THROW(
        actual = client
                     ->Execute(storages::sqlite::OperationType::kReadOnly,
                               kSelectNullRow.data())
                     .AsVector<Row>());

    EXPECT_TRUE(actual.empty());
  }

  // Get result as a single field
  {
    std::string actual;
    UEXPECT_NO_THROW(
        actual = client
                     ->Execute(storages::sqlite::OperationType::kReadOnly,
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
    UEXPECT_NO_THROW(
        actual = client
                     ->Execute(storages::sqlite::OperationType::kReadOnly,
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
                      ->Execute(storages::sqlite::OperationType::kReadOnly,
                                kSelectAllRows.data())
                      .AsVector<std::string>(kFieldTag),
                  SQLiteException);
  }

  // Throw exception if try to get single row from empty result
  {
    UEXPECT_THROW(client
                      ->Execute(storages::sqlite::OperationType::kReadOnly,
                                kSelectNullRow.data())
                      .AsSingleRow<Row>(),
                  SQLiteException);
  }

  // Throw exception if result is empty
  {
    UEXPECT_THROW(client
                      ->Execute(storages::sqlite::OperationType::kReadOnly,
                                kSelectNullField.data())
                      .AsSingleField<std::string>(),
                  SQLiteException);
  }

  // Select with unexpected fields
  {
    std::vector<RowTuple> actual;
    UEXPECT_THROW(
        actual = client
                     ->Execute(storages::sqlite::OperationType::kReadOnly,
                               kUnexpectedFieldsSelect.data())
                     .AsVector<RowTuple>(),
        SQLiteException);
  }

  // Insert unexpected fields (datatype mismatch)
  {
    UEXPECT_THROW(client->Execute(storages::sqlite::OperationType::kReadOnly,
                                  kDatatypeMismatchInsert.data()),
                  SQLiteException);
  }
}

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
