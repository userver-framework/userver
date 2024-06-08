#include <userver/utest/utest.hpp>

#include "small_table.hpp"
#include "test_utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace {
class YdbScanTable : public YdbSmallTableTest {};
}  // namespace

UTEST_F(YdbScanTable, Simple) {
  CreateTable("test_table", true);

  auto results = GetTableClient().ExecuteScanQuery(
      "SELECT * FROM test_table ORDER BY key");

  auto cursor = results.GetNextCursor();
  ASSERT_TRUE(cursor);
  AssertArePreFilledRows(std::move(*cursor), {1, 2, 3});

  while (cursor = results.GetNextCursor()) {
    EXPECT_THAT(*cursor, testing::IsEmpty());
  }
}

UTEST_F(YdbScanTable, Diagnostics) {
  CreateTable("test_table", true);

  auto scan_settings =
      ydb::ScanQuerySettings{}
          .CollectQueryStats(NYdb::NTable::ECollectQueryStatsMode::Full)
          .CollectFullDiagnostics(true);
  auto results = GetTableClient().ExecuteScanQuery(
      std::move(scan_settings),
      /*settings=*/{}, "SELECT * FROM test_table ORDER BY key");

  auto result = results.GetNextResult();
  ASSERT_TRUE(result);
  EXPECT_TRUE(result->HasResultSet());
  EXPECT_TRUE(result->HasDiagnostics());
  EXPECT_FALSE(result->HasQueryStats());
  AssertArePreFilledRows(ydb::Cursor{result->ExtractResultSet()}, {1, 2, 3});

  while (true) {
    result = results.GetNextResult();
    ASSERT_TRUE(result.has_value());
    if (!result->HasResultSet()) break;
  }

  EXPECT_FALSE(result->HasResultSet());
  EXPECT_TRUE(result->HasDiagnostics());
  EXPECT_TRUE(result->HasQueryStats());

  EXPECT_FALSE(results.GetNextResult().has_value());
}

USERVER_NAMESPACE_END
