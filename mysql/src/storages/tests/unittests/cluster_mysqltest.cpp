#include <userver/utest/utest.hpp>
#include "../utils_mysqltest.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::tests {

namespace {

constexpr auto kPrimaryHost = storages::mysql::ClusterHostType::kPrimary;

struct Row final {
  int id{};
  std::string value;

  bool operator==(const Row& other) const {
    return id == other.id && value == other.value;
  }
};

struct Id final {
  std::int32_t id{};
};

}  // namespace

UTEST(StreamedResult, Works) {
  ClusterWrapper cluster{};

  TmpTable table{cluster, "Id INT NOT NULL, Value TEXT NOT NULL"};

  constexpr std::size_t kRowsCount = 10;
  std::vector<Row> rows_to_insert;
  rows_to_insert.reserve(kRowsCount);

  for (std::size_t i = 0; i < kRowsCount; ++i) {
    rows_to_insert.push_back(
        {static_cast<std::int32_t>(i), utils::generators::GenerateUuid()});

    cluster->ExecuteDecompose(
        ClusterHostType::kPrimary,
        table.FormatWithTableName("INSERT INTO {}(Id, Value) VALUES(?, ?)"),
        rows_to_insert.back());
  }

  std::vector<Row> db_rows;
  db_rows.reserve(kRowsCount);

  cluster
      ->GetCursor<Row>(kPrimaryHost, 3,
                       table.FormatWithTableName("SELECT Id, Value FROM {}"))
      .ForEach([&db_rows](Row&& row) { db_rows.push_back(std::move(row)); },
               cluster.GetDeadline());

  auto select_as_vector =
      table.DefaultExecute("SELECT Id, Value FROM {}").AsVector<Row>();

  ASSERT_EQ(db_rows.size(), rows_to_insert.size());
  EXPECT_EQ(db_rows, rows_to_insert);
  ASSERT_EQ(select_as_vector.size(), rows_to_insert.size());
}

UTEST(Cluster, InsertMany) {
  ClusterWrapper cluster{};
  TmpTable table{cluster, "Id INT NOT NULL, Value TEXT NOT NULL"};

  const std::string long_string_to_avoid_sso{
      "hi i am some long string that doesn't fit in sso"};

  constexpr int kRowsCount = 10;

  std::vector<Row> rows_to_insert;
  rows_to_insert.reserve(kRowsCount);
  for (int i = 0; i < kRowsCount; ++i) {
    rows_to_insert.push_back(
        {i, fmt::format("{}: {}", i, long_string_to_avoid_sso)});
  }

  cluster->ExecuteBulk(
      ClusterHostType::kPrimary,
      table.FormatWithTableName("INSERT INTO {}(Id, Value) VALUES(?, ?)"),
      rows_to_insert);

  const auto db_rows =
      table.DefaultExecute("SELECT Id, Value FROM {}").AsVector<Row>();
  EXPECT_EQ(db_rows, rows_to_insert);
}

UTEST(Cluster, UpdateMany) {
  ClusterWrapper cluster{};
  TmpTable table{cluster, "Id INT PRIMARY KEY, Value TEXT NOT NULL"};

  const std::string long_string_to_avoid_sso{
      "hi i am some long string that doesn't fit in sso"};
  const std::string another_long_string{"hi i am another pretty long string"};

  constexpr int kRowsCount = 100;

  std::vector<Row> rows_to_insert;
  rows_to_insert.reserve(kRowsCount);
  for (int i = 0; i < kRowsCount; ++i) {
    rows_to_insert.push_back({i, long_string_to_avoid_sso});
  }

  cluster->ExecuteBulk(
      ClusterHostType::kPrimary,
      table.FormatWithTableName("INSERT INTO {}(Id, Value) VALUES(?, ?)"),
      rows_to_insert);

  {
    const auto db_rows =
        table.DefaultExecute("SELECT Id, Value FROM {}").AsVector<Row>();
    EXPECT_EQ(db_rows, rows_to_insert);
  }

  for (auto& row : rows_to_insert) {
    row.value = another_long_string;
  }
  cluster->ExecuteBulk(
      ClusterHostType::kPrimary,
      table.FormatWithTableName("INSERT INTO {}(Id, Value) VALUES(?, ?) ON "
                                "DUPLICATE KEY UPDATE Value=VALUES(Value)"),
      rows_to_insert);
  {
    const auto db_rows =
        table.DefaultExecute("SELECT Id, Value FROM {}").AsVector<Row>();
    EXPECT_EQ(db_rows, rows_to_insert);
  }
}

UTEST(ShowCase, Basic) {
  ClusterWrapper cluster{};

  TmpTable table{
      cluster,
      "Id INT NOT NULL, Value TEXT NOT NULL, CreatedAt DATETIME(6) NOT NULL"};

  struct Row final {
    std::int32_t id{};
    std::string value;
    std::chrono::system_clock::time_point created_at;
  };
  const std::vector<Row> rows =
      cluster
          ->Execute(
              ClusterHostType::kPrimary,
              table.FormatWithTableName(
                  "SELECT Id, Value, CreatedAt FROM {} WHERE CreatedAt > ?"),
              std::chrono::system_clock::now() - std::chrono::hours{3})
          .AsVector<Row>();

  table.DefaultExecute("INSERT INTO {} VALUES(?, ?, ?)", 1, "we",
                       std::chrono::system_clock::now());

  const std::vector<Row> we =
      cluster
          ->Execute(
              ClusterHostType::kPrimary,
              table.FormatWithTableName(
                  "SELECT Id, Value, CreatedAt FROM {} WHERE CreatedAt > ?"),
              std::chrono::system_clock::now() - std::chrono::hours{3})
          .AsVector<Row>();
}

UTEST(ShowCase, BatchInsert) {
  ClusterWrapper cluster{};
  TmpTable table{cluster, "Id INT NOT NULL, Value TEXT NOT NULL"};

  struct Row final {
    std::int32_t id;
    std::string value;
  };

  constexpr int kRowsCount = 100;
  std::vector<Row> rows_to_insert;
  rows_to_insert.reserve(kRowsCount);
  for (int i = 0; i < kRowsCount; ++i) {
    rows_to_insert.push_back(
        {i, "i am just some random string, don't mind me"});
  }

  cluster->ExecuteBulk(
      ClusterHostType::kPrimary,
      table.FormatWithTableName("INSERT INTO {}(Id, Value) VALUES(?, ?)"),
      rows_to_insert);
}

namespace {

struct DbRow final {
  std::int32_t id{};
  std::string username;
};

struct UserRow final {
  std::int32_t id{};
  std::string first_name;
  std::string last_name;
};

DbRow Convert(const UserRow& user_row, storages::mysql::convert::To<DbRow>) {
  return DbRow{user_row.id,
               fmt::format("{} {}", user_row.first_name, user_row.last_name)};
}

}  // namespace

UTEST(Cluster, MappedBatchInsert) {
  ClusterWrapper cluster{};
  TmpTable table{cluster, "Id INT NOT NULL, Username TEXT NOT NULL"};
  std::vector<UserRow> users{{1, "Ivan", "Trofimov"}, {2, "John", "Doe"}};

  cluster->ExecuteBulkMapped<DbRow>(
      ClusterHostType::kPrimary,
      table.FormatWithTableName("INSERT INTO {}(Id, Username) VALUES(?, ?)"),
      users);

  const auto db_rows =
      table.DefaultExecute("SELECT Id, Username FROM {}").AsVector<DbRow>();
  ASSERT_EQ(db_rows.size(), 2);
  EXPECT_EQ(db_rows[0].username, "Ivan Trofimov");
  EXPECT_EQ(db_rows[1].username, "John Doe");
}

namespace {
/// [uMySQL usage sample - Cluster ExecuteCommand]
void PrepareExampleTable(const Cluster& cluster) {
  constexpr std::chrono::milliseconds kTimeout{1750};
  cluster.ExecuteCommand(ClusterHostType::kPrimary,
                         "DROP TABLE IF EXISTS SampleTable");
  cluster.ExecuteCommand(CommandControl{kTimeout}, ClusterHostType::kPrimary,
                         "CREATE TABLE SampleTable("
                         "Id INT PRIMARY KEY AUTO_INCREMENT,"
                         "title TEXT NOT NULL,"
                         "amount INT NOT NULL,"
                         "created TIMESTAMP NOT NULL)");
}
/// [uMySQL usage sample - Cluster ExecuteCommand]
}  // namespace

namespace execute_sample {

/// [uMySQL usage sample - Cluster Execute]
void PerformExecute(const Cluster& cluster, std::chrono::milliseconds timeout,
                    const std::string& title, int amount) {
  const auto insertion_result =
      cluster
          .Execute(CommandControl{timeout}, ClusterHostType::kPrimary,
                   "INSERT INTO SampleTable(title, amount, created) "
                   "VALUES(?, ?, ?)",
                   title, amount, std::chrono::system_clock::now())
          .AsExecutionResult();

  EXPECT_EQ(insertion_result.rows_affected, 1);
  EXPECT_EQ(insertion_result.last_insert_id, 1);
}
/// [uMySQL usage sample - Cluster Execute]

UTEST(Cluster, Execute) {
  const ClusterWrapper cluster{};

  PrepareExampleTable(*cluster);

  PerformExecute(*cluster, std::chrono::milliseconds{1750}, "title", 1);
}

}  // namespace execute_sample

namespace execute_decompose_sample {

/// [uMySQL usage sample - Cluster ExecuteDecompose]
struct SampleRow final {
  std::string title;
  int amount;
  std::chrono::system_clock::time_point created;
};

void PerformExecuteDecompose(const Cluster& cluster,
                             std::chrono::milliseconds timeout,
                             const SampleRow& row) {
  const auto insertion_result =
      cluster
          .ExecuteDecompose(CommandControl{timeout}, ClusterHostType::kPrimary,
                            "INSERT INTO SampleTable(title, amount, created) "
                            "VALUES(?, ?, ?)",
                            row)
          .AsExecutionResult();

  EXPECT_EQ(insertion_result.rows_affected, 1);
  EXPECT_EQ(insertion_result.last_insert_id, 1);
}
/// [uMySQL usage sample - Cluster ExecuteDecompose]

UTEST(Cluster, ExecuteDecompose) {
  const ClusterWrapper cluster{};

  PrepareExampleTable(*cluster);

  PerformExecuteDecompose(
      *cluster, std::chrono::milliseconds{1750},
      SampleRow{"title", 2, std::chrono::system_clock::now()});
}

}  // namespace execute_decompose_sample

namespace execute_bulk_sample {

/// [uMySQL usage sample - Cluster ExecuteBulk]
struct SampleRow final {
  std::string title;
  int amount;
  std::chrono::system_clock::time_point created;
};

void PerformExecuteBulk(const Cluster& cluster,
                        std::chrono::milliseconds timeout,
                        const std::vector<SampleRow>& rows) {
  const auto bulk_insertion_result =
      cluster
          .ExecuteBulk(CommandControl{timeout}, ClusterHostType::kPrimary,
                       "INSERT INTO SampleTable(title, amount, created) "
                       "VALUES(?, ?, ?)",
                       rows)
          .AsExecutionResult();

  // When performing a multi insert prepared statement, mysql_stmt_insert_id()
  // will return the value of the first row.
  EXPECT_EQ(bulk_insertion_result.last_insert_id, 1);
  EXPECT_EQ(bulk_insertion_result.rows_affected, rows.size());
}
/// [uMySQL usage sample - Cluster ExecuteBulk]

UTEST(Cluster, ExecuteBulk) {
  const ClusterWrapper cluster{};

  PrepareExampleTable(*cluster);

  constexpr std::size_t kRowsCount = 7;
  std::vector<SampleRow> rows;
  rows.reserve(kRowsCount);
  for (std::size_t i = 0; i < kRowsCount; ++i) {
    rows.push_back(
        SampleRow{std::to_string(i), 1, std::chrono::system_clock::now()});
  }

  PerformExecuteBulk(*cluster, std::chrono::milliseconds{1750}, rows);
}

}  // namespace execute_bulk_sample

namespace execute_bulk_mapped_sample {

/// [uMySQL usage sample - Cluster ExecuteBulkMapped]
struct SampleUserStruct final {
  std::string name;
  std::string description;
  int amount;
};

struct SampleRow final {
  std::string title;
  int amount;
  std::chrono::system_clock::time_point created;
};

SampleRow Convert(const SampleUserStruct& data, convert::To<SampleRow>) {
  return {
      fmt::format("{} {}", data.name, data.description),  // title
      data.amount,                                        // amount
      std::chrono::system_clock::now()                    // created
  };
}

void PerformExecuteBulkMapped(const Cluster& cluster,
                              std::chrono::milliseconds timeout,
                              const std::vector<SampleUserStruct>& data) {
  const auto bulk_insertion_result =
      cluster
          .ExecuteBulkMapped<SampleRow>(
              CommandControl{timeout}, ClusterHostType::kPrimary,
              "INSERT INTO SampleTable(title, amount, created) "
              "VALUES(?, ?, ?)",
              data)
          .AsExecutionResult();

  // When performing a multi insert prepared statement, mysql_stmt_insert_id()
  // will return the value of the first row.
  EXPECT_EQ(bulk_insertion_result.last_insert_id, 1);
  EXPECT_EQ(bulk_insertion_result.rows_affected, data.size());
}
/// [uMySQL usage sample - Cluster ExecuteBulkMapped]

UTEST(Cluster, ExecuteBulkMapped) {
  const ClusterWrapper cluster{};

  PrepareExampleTable(*cluster);

  constexpr std::size_t kRowsCount = 7;
  std::vector<SampleUserStruct> data;
  data.reserve(kRowsCount);
  for (std::size_t i = 0; i < kRowsCount; ++i) {
    data.push_back({"name", std::to_string(i), 2});
  }

  PerformExecuteBulkMapped(*cluster, std::chrono::milliseconds{1750}, data);
}

}  // namespace execute_bulk_mapped_sample

namespace get_cursor_sample {

/// [uMySQL usage sample - Cluster GetCursor]
struct SampleRow final {
  std::string title;
  int amount;
  std::chrono::system_clock::time_point created;
};

void PerformGetCursor(const Cluster& cluster,
                      std::chrono::milliseconds timeout) {
  const std::size_t batch_size = 5;
  const auto deadline = engine::Deadline::FromDuration(timeout);

  std::int64_t total = 0;
  cluster
      .GetCursor<SampleRow>(CommandControl{timeout},
                            ClusterHostType::kSecondary, batch_size,
                            "SELECT title, amount, created FROM SampleTable")
      .ForEach([&total](SampleRow&& row) { total += row.amount; }, deadline);

  // There are 9 rows in the table with amount from 1 to 9
  EXPECT_EQ(total, 45);
}
/// [uMySQL usage sample - Cluster GetCursor]

UTEST(Cluster, GetCursor) {
  const ClusterWrapper cluster{};

  PrepareExampleTable(*cluster);

  {
    constexpr std::size_t kRowsCount = 9;
    std::vector<execute_bulk_sample::SampleRow> rows{};
    rows.reserve(kRowsCount);
    for (std::size_t i = 1; i <= kRowsCount; ++i) {
      rows.push_back({std::to_string(i), static_cast<int>(i),
                      std::chrono::system_clock::now()});
    }
    execute_bulk_sample::PerformExecuteBulk(
        *cluster, std::chrono::milliseconds{1750}, rows);
  }

  PerformGetCursor(*cluster, std::chrono::milliseconds{1750});
}

}  // namespace get_cursor_sample

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END
