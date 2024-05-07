#include <boost/pfr/ops_fields.hpp>

#include <userver/utest/utest.hpp>

#include "small_table.hpp"
#include "test_utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace {
class YdbStructFromRow : public YdbSmallTableTest {};
}  // namespace

UTEST_F(YdbStructFromRow, StructReadRow) {
  CreateTable("test_table", true);

  const ydb::Query kSelectQuery{R"(
    --!syntax_v1

    SELECT value_int, key, value_str
    FROM test_table
    ORDER BY key;
  )"};
  auto cursor =
      GetTableClient().ExecuteDataQuery(kSelectQuery).GetSingleCursor();
  ASSERT_FALSE(cursor.IsTruncated());

  ASSERT_EQ(cursor.size(), 3);
  for (auto [index, row] : utils::enumerate(cursor)) {
    const auto item = std::move(row).As<tests::RowValue>();
    EXPECT_TRUE(boost::pfr::eq_fields(item, kPreFilledRows[index]));
  }
}

namespace tests {

struct StructReadRowMissingColumn {
  std::string key;
  std::string value_str;
  std::int32_t value_int;
  bool extra_value;
};

}  // namespace tests

template <>
inline constexpr auto
    ydb::kStructMemberNames<tests::StructReadRowMissingColumn> =
        ydb::StructMemberNames{};

UTEST_F(YdbStructFromRow, StructReadRowMissingColumn) {
  CreateTable("test_table", true);

  const ydb::Query kSelectQuery{R"(
    --!syntax_v1

    SELECT key, value_str, value_int
    FROM test_table
    ORDER BY key;
  )"};
  auto cursor =
      GetTableClient().ExecuteDataQuery(kSelectQuery).GetSingleCursor();
  ASSERT_FALSE(cursor.IsTruncated());

  ASSERT_EQ(cursor.size(), 3);
  UEXPECT_THROW_MSG(
      cursor.GetFirstRow().As<tests::StructReadRowMissingColumn>(),
      ydb::ParseError, "Missing column 'extra_value'");
}

namespace tests {

struct StructReadRowExtraColumn {
  std::string key;
  std::string value_str;
};

}  // namespace tests

template <>
inline constexpr auto ydb::kStructMemberNames<tests::StructReadRowExtraColumn> =
    ydb::StructMemberNames{};

UTEST_F(YdbStructFromRow, StructReadRowExtraColumn) {
  CreateTable("test_table", true);

  const ydb::Query kSelectQuery{R"(
    --!syntax_v1

    SELECT key, value_str, value_int
    FROM test_table
    ORDER BY key;
  )"};
  auto cursor =
      GetTableClient().ExecuteDataQuery(kSelectQuery).GetSingleCursor();
  ASSERT_FALSE(cursor.IsTruncated());

  ASSERT_EQ(cursor.size(), 3);
  UEXPECT_THROW_MSG(cursor.GetFirstRow().As<tests::StructReadRowExtraColumn>(),
                    ydb::ParseError, "Unexpected extra columns");
}

USERVER_NAMESPACE_END
