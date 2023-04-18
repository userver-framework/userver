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

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END
