#include <unordered_set>
#include <vector>

#include <gmock/gmock.h>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/irange.hpp>

#include <userver/utest/utest.hpp>

#include "small_table.hpp"
#include "test_utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace {
class YdbListIO : public YdbSmallTableTest {};
}  // namespace

UTEST_F(YdbListIO, ReadVector) {
  auto response = GetTableClient().ExecuteDataQuery(R"(
    SELECT AsList(1, 2, 3, 4, 5);
  )");

  auto cursor = response.GetSingleCursor();
  const auto list = cursor.GetFirstRow().Get<std::vector<std::int32_t>>(0);
  EXPECT_THAT(list, testing::ElementsAreArray({1, 2, 3, 4, 5}));
}

UTEST_F(YdbListIO, ReadUnorderedSet) {
  auto response = GetTableClient().ExecuteDataQuery(R"(
    SELECT AsList(1, 2, 3, 4, 5);
  )");

  auto cursor = response.GetSingleCursor();
  const auto list =
      cursor.GetFirstRow().Get<std::unordered_set<std::int32_t>>(0);
  EXPECT_THAT(list, testing::UnorderedElementsAreArray({1, 2, 3, 4, 5}));
}

UTEST_F(YdbListIO, BulkUpsertVectorOfStructs) {
  CreateTable("test_table", false);

  GetTableClient().BulkUpsert("test_table", kPreFilledRows);

  auto result = GetTableClient().ExecuteDataQuery(kSelectAllRows);
  AssertArePreFilledRows(result.GetSingleCursor(), {1, 2, 3});
}

UTEST_F(YdbListIO, BulkUpsertEmptyVectorOfStructs) {
  CreateTable("test_table", false);

  GetTableClient().BulkUpsert("test_table", std::vector<tests::RowValue>{});

  auto result = GetTableClient().ExecuteDataQuery(kSelectAllRows);
  AssertArePreFilledRows(result.GetSingleCursor(), {});
}

UTEST_F(YdbListIO, BulkUpsertRangeOfStructs) {
  CreateTable("test_table", false);

  auto serialized_rows =
      boost::irange(0, 3) |
      boost::adaptors::transformed([](int i) { return kPreFilledRows[i]; });
  GetTableClient().BulkUpsert("test_table", std::move(serialized_rows));

  auto result = GetTableClient().ExecuteDataQuery(kSelectAllRows);
  AssertArePreFilledRows(result.GetSingleCursor(), {1, 2, 3});
}

UTEST_F(YdbListIO, BulkUpsertEmptyRangeOfStructs) {
  CreateTable("test_table", false);

  auto serialized_rows =
      boost::irange(0, 0) |
      boost::adaptors::transformed([](int i) { return kPreFilledRows[i]; });
  GetTableClient().BulkUpsert("test_table", std::move(serialized_rows));

  auto result = GetTableClient().ExecuteDataQuery(kSelectAllRows);
  AssertArePreFilledRows(result.GetSingleCursor(), {});
}

USERVER_NAMESPACE_END
