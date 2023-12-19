#include <userver/utest/utest.hpp>
#include "../utils_mysqltest.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::tests {

namespace {

struct Row final {
  std::int32_t id{};
  std::string value;

  bool operator==(const Row& other) const {
    return std::tie(id, value) == std::tie(other.id, other.value);
  }
};

}  // namespace

UTEST(Cursor, Works) {
  ClusterWrapper cluster{};
  TmpTable table{cluster, "Id INT NOT NULL, Value TEXT NOT NULL"};

  constexpr std::size_t rows_count = 20;
  std::vector<Row> rows_to_insert;
  rows_to_insert.reserve(rows_count);

  for (std::size_t i = 0; i < rows_count; ++i) {
    rows_to_insert.push_back(
        {static_cast<std::int32_t>(i), utils::generators::GenerateUuid()});

    cluster->ExecuteDecompose(
        ClusterHostType::kPrimary,
        table.FormatWithTableName("INSERT INTO {}(Id, Value) VALUES(?, ?)"),
        rows_to_insert.back());
  }

  std::vector<Row> db_rows;
  db_rows.reserve(rows_count);

  cluster
      ->GetCursor<Row>(ClusterHostType::kPrimary, 7,
                       table.FormatWithTableName("SELECT Id, Value FROM {}"))
      .ForEach([&db_rows](Row&& row) { db_rows.push_back(std::move(row)); },
               cluster.GetDeadline());
  EXPECT_EQ(db_rows, rows_to_insert);
}

// https://bugs.mysql.com/bug.php?id=109380
UTEST(Cursor, StatementReuseWorks) {
  ClusterWrapper cluster{};
  TmpTable table{cluster, "Id INT NOT NULL, Value TEXT NOT NULL"};

  constexpr std::size_t rows_count = 1;
  std::vector<Row> rows_to_insert;
  rows_to_insert.reserve(rows_count);

  for (std::size_t i = 0; i < rows_count; ++i) {
    rows_to_insert.push_back(
        {static_cast<std::int32_t>(i), utils::generators::GenerateUuid()});

    cluster->ExecuteDecompose(
        ClusterHostType::kPrimary,
        table.FormatWithTableName("INSERT INTO {}(Id, Value) VALUES(?, ?)"),
        rows_to_insert.back());
  }

  std::vector<Row> cursor_rows;
  cursor_rows.reserve(rows_count);

  cluster
      ->GetCursor<Row>(ClusterHostType::kPrimary, 2,
                       table.FormatWithTableName("SELECT Id, Value FROM {}"))
      .ForEach(
          [&cursor_rows](Row&& row) { cursor_rows.push_back(std::move(row)); },
          cluster.GetDeadline());
  EXPECT_EQ(cursor_rows, rows_to_insert);

  const auto db_rows =
      table.DefaultExecute("SELECT Id, Value FROM {}").AsVector<Row>();
  EXPECT_EQ(db_rows, rows_to_insert);
}

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END
