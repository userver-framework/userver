#include <userver/utest/utest.hpp>
#include "../utils_mysqltest.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::tests {

UTEST(Cursor, Works) {
  ClusterWrapper cluster{};
  TmpTable table{cluster, "Id INT NOT NULL, Value TEXT NOT NULL"};

  struct Row final {
    std::int32_t id{};
    std::string value;

    bool operator==(const Row& other) const {
      return std::tie(id, value) == std::tie(other.id, other.value);
    }
  };

  constexpr std::size_t rows_count = 1;
  std::vector<Row> rows_to_insert;
  rows_to_insert.reserve(rows_count);

  for (std::size_t i = 0; i < rows_count; ++i) {
    rows_to_insert.push_back(
        {static_cast<std::int32_t>(i), utils::generators::GenerateUuid()});

    cluster->InsertOne(
        cluster.GetDeadline(),
        table.FormatWithTableName("INSERT INTO {}(Id, Value) VALUES(?, ?)"),
        rows_to_insert.back());
  }

  /*cluster->InsertMany(
      cluster.GetDeadline(),
      table.FormatWithTableName("INSERT INTO {}(Id, Value) VALUES(?, ?)"),
      rows_to_insert);*/

  std::vector<Row> db_rows;
  db_rows.reserve(rows_count);

  // auto asd = table.DefaultExecute("SELECT Id, Value FROM
  // {}").AsVector<Row>();

  cluster
      ->GetCursor<Row>(ClusterHostType::kMaster, cluster.GetDeadline(), 2,
                       table.FormatWithTableName("SELECT Id, Value FROM {}"))
      .ForEach([&db_rows](Row&& row) { db_rows.push_back(std::move(row)); },
               cluster.GetDeadline());
  ASSERT_EQ(db_rows.size(), rows_to_insert.size());
  EXPECT_EQ(db_rows, rows_to_insert);

  auto we = table.DefaultExecute("SELECT Id, Value FROM {}").AsVector<Row>();
  ASSERT_EQ(we.size(), rows_to_insert.size());
}

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END