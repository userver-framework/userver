#include <userver/utest/utest.hpp>
#include "../utils_mysqltest.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::tests {

namespace {

struct RowToInsert final {
  std::string_view value;
};
struct Row {
  std::string value;
};

}  // namespace

UTEST(StringView, WorksWithSingleInsert) {
  TmpTable table{"Value TEXT NOT NULL"};

  const RowToInsert row_to_insert{"hi from std::string_view"};
  table.GetCluster()->ExecuteDecompose(
      ClusterHostType::kPrimary,
      table.FormatWithTableName("INSERT INTO {} VALUES(?)"), row_to_insert);

  const auto db_row =
      table.DefaultExecute("SELECT Value FROM {}").AsSingleRow<Row>();
  EXPECT_EQ(row_to_insert.value, db_row.value);
}

UTEST(StringView, WorksWithBatchInsert) {
  TmpTable table{"Value TEXT NOT NULL"};

  std::vector<RowToInsert> rows_to_insert{{"first value"}, {"second value"}};
  table.GetCluster()->ExecuteBulk(
      ClusterHostType::kPrimary,
      table.FormatWithTableName("INSERT INTO {} VALUES(?)"), rows_to_insert);

  const auto db_rows =
      table.DefaultExecute("SELECT Value FROM {}").AsVector<Row>();
  ASSERT_EQ(rows_to_insert.size(), db_rows.size());
  ASSERT_EQ(rows_to_insert.size(), 2);
  EXPECT_EQ(rows_to_insert[0].value, db_rows[0].value);
  EXPECT_EQ(rows_to_insert[1].value, db_rows[1].value);
}

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END
