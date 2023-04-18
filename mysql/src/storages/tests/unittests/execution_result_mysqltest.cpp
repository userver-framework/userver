#include <userver/utest/utest.hpp>
#include "../utils_mysqltest.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::tests {

namespace {

struct DbRow final {
  std::int32_t id{};
};

struct DbRowIdValue final {
  std::int32_t id{};
  std::string value;

  bool operator==(const DbRowIdValue& other) const {
    return id == other.id && value == other.value;
  }
};

DbRow Convert(std::int32_t id, storages::mysql::convert::To<DbRow>) {
  return {id};
}

}  // namespace

UTEST(ExecutionResult, SingleInsert) {
  TmpTable table{"Id INT NOT NULL"};
  const auto res =
      table.DefaultExecute("INSERT INTO {} VALUES(?)", 1).AsExecutionResult();
  EXPECT_EQ(res.rows_affected, 1);
  EXPECT_EQ(res.last_insert_id, 0);
}

UTEST(ExecutionResult, BatchInsert) {
  ClusterWrapper cluster{};
  TmpTable table{"ID INT NOT NULL"};

  std::vector<std::int32_t> ids{1, 2, 3, 4, 5, 6, 7};
  const auto res =
      cluster
          ->ExecuteBulkMapped<DbRow>(
              ClusterHostType::kPrimary,
              table.FormatWithTableName("INSERT INTO {} VALUES(?)"), ids)
          .AsExecutionResult();

  EXPECT_EQ(res.rows_affected, 7);
  EXPECT_EQ(res.last_insert_id, 0);
}

UTEST(ExecutionResult, SingleUpdate) {
  TmpTable table{
      "Id INT NOT NULL AUTO_INCREMENT, Value TEXT NOT NULL, PRIMARY KEY(Id)"};
  const auto insert_res =
      table.DefaultExecute("INSERT INTO {}(Value) VALUES(?)", "text")
          .AsExecutionResult();
  EXPECT_EQ(insert_res.rows_affected, 1);
  EXPECT_EQ(insert_res.last_insert_id, 1);

  const auto update_res =
      table
          .DefaultExecute(
              "INSERT INTO {} VALUES(?, ?) "
              "ON DUPLICATE KEY UPDATE Value = VALUES(Value);",
              1, "other text")
          .AsExecutionResult();
  // The REPLACE statement first deletes the record with the same primary key
  // and then inserts the new record. This function returns the number of
  // deleted records in addition to the number of inserted records.
  // Guess this is replace basically
  EXPECT_EQ(update_res.rows_affected, 2);
  EXPECT_EQ(update_res.last_insert_id, 1);

  const auto no_update_res =
      table
          .DefaultExecute(
              "INSERT INTO {} VALUES(?, ?) "
              "ON DUPLICATE KEY UPDATE Value = VALUES(Value);",
              1, "other text")
          .AsExecutionResult();
  // When using UPDATE, MariaDB will not update columns where the new value is
  // the same as the old value. This creates the possibility that
  // mysql_stmt_affected_rows() may not actually equal the number of rows
  // matched, only the number of rows that were literally affected by the query.
  EXPECT_EQ(no_update_res.rows_affected, 0);
  EXPECT_EQ(no_update_res.last_insert_id, 0);

  const auto no_update_res_2 =
      table
          .DefaultExecute("UPDATE {} SET Value = ? WHERE ID = ?", "other text",
                          1)
          .AsExecutionResult();
  EXPECT_EQ(no_update_res_2.rows_affected, 0);
  EXPECT_EQ(no_update_res_2.last_insert_id, 0);
}

UTEST(ExecutionResult, BatchUpdate) {
  ClusterWrapper cluster{};
  TmpTable table{
      cluster,
      "Id INT NOT NULL AUTO_INCREMENT, Value TEXT NOT NULL, PRIMARY KEY(Id)"};

  constexpr int kRowsCount = 10;
  struct DbValue final {
    std::string value;
  };
  std::vector<DbValue> values;
  values.reserve(kRowsCount);
  for (int i = 0; i < kRowsCount; ++i) {
    values.push_back(DbValue{"some text"});
  }

  const auto insert_res =
      cluster
          ->ExecuteBulk(
              ClusterHostType::kPrimary,
              table.FormatWithTableName("INSERT INTO {}(Value) VALUES(?)"),
              values)
          .AsExecutionResult();
  EXPECT_EQ(insert_res.rows_affected, kRowsCount);
  // When performing a multi insert prepared statement, mysql_stmt_insert_id()
  // will return the value of the first row.
  // Oh well
  EXPECT_EQ(insert_res.last_insert_id, 1);

  const storages::mysql::Query upsert_query{table.FormatWithTableName(
      "INSERT INTO {} VALUES(?, ?) "
      "ON DUPLICATE KEY UPDATE Value = VALUES(Value);")};

  auto db_rows =
      table.DefaultExecute("SELECT Id, Value FROM {}").AsVector<DbRowIdValue>();

  const auto no_update_res =
      cluster->ExecuteBulk(ClusterHostType::kPrimary, upsert_query, db_rows)
          .AsExecutionResult();
  EXPECT_EQ(no_update_res.rows_affected, 0);
  EXPECT_EQ(no_update_res.last_insert_id, 0);

  for (auto& [k, v] : db_rows) {
    v.push_back('a');
  }
  const auto update_res =
      cluster->ExecuteBulk(ClusterHostType::kPrimary, upsert_query, db_rows)
          .AsExecutionResult();
  EXPECT_EQ(update_res.rows_affected, kRowsCount * 2);
  EXPECT_EQ(update_res.last_insert_id, kRowsCount);

  EXPECT_EQ(db_rows, table.DefaultExecute("SELECT Id, Value FROM {}")
                         .AsVector<DbRowIdValue>());
}

UTEST(ExecutionResult, WithSelect) {
  TmpTable table{"Id INT, Value TEXT"};
  const auto res =
      table.DefaultExecute("SELECT Id, Value FROM {}").AsExecutionResult();

  EXPECT_EQ(res.rows_affected, 0);
  EXPECT_EQ(res.last_insert_id, 0);
}

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END
