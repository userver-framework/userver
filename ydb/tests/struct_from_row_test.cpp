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

    const ydb::Query select_query{R"(
    --!syntax_v1

    SELECT value_int, key, value_str
    FROM test_table
    ORDER BY key;
  )"};
    auto cursor = GetTableClient().ExecuteDataQuery(select_query).GetSingleCursor();
    ASSERT_FALSE(cursor.IsTruncated());

    ASSERT_EQ(cursor.size(), 3);
    for (auto [index, row] : utils::enumerate(cursor)) {
        const auto item = std::move(row).As<tests::RowValue>();
        EXPECT_TRUE(boost::pfr::eq_fields(item, kPreFilledRows[index]));
    }
}

UTEST_F(YdbStructFromRow, StructReadRowAsContainer) {
    CreateTable("test_table", true);

    const ydb::Query select_query{R"(
    --!syntax_v1

    SELECT value_int, key, value_str
    FROM test_table
    ORDER BY key;
  )"};
    auto result = GetTableClient().ExecuteDataQuery(select_query);
    ASSERT_EQ(result.GetSingleCursor().AsContainer<std::vector<tests::RowValue>>(), kPreFilledRows);
}

UTEST_F(YdbStructFromRow, AsSingleRow) {
    CreateTable("test_table", true);

    const ydb::Query select_query{R"(
    --!syntax_v1

    SELECT value_int, key, value_str
    FROM test_table
    WHERE key = "key1"
    ORDER BY key;
  )"};
    auto result = GetTableClient().ExecuteDataQuery(select_query);
    ASSERT_EQ(result.GetSingleCursor().AsSingleRow<tests::RowValue>(), kPreFilledRows[0]);
}

UTEST_F(YdbStructFromRow, AsSingleRowThrowEmptyResponseError) {
    CreateTable("test_table", false);

    const ydb::Query select_query{R"(
    --!syntax_v1

    SELECT value_int, key, value_str
    FROM test_table
    WHERE FALSE;
  )"};
    auto cursor = GetTableClient().ExecuteDataQuery(select_query).GetSingleCursor();
    UASSERT_THROW(std::move(cursor).AsSingleRow<tests::RowValue>(), ydb::EmptyResponseError);
}

UTEST_F(YdbStructFromRow, AsSingleRowThrowIgnoreResultsError) {
    CreateTable("test_table", true);

    const ydb::Query select_query{R"(
    --!syntax_v1

    SELECT value_int, key, value_str
    FROM test_table;
  )"};
    auto cursor = GetTableClient().ExecuteDataQuery(select_query).GetSingleCursor();
    UASSERT_THROW(std::move(cursor).AsSingleRow<tests::RowValue>(), ydb::IgnoreResultsError);
}

UTEST_F(YdbStructFromRow, AsOptionalSingleRow) {
    CreateTable("test_table", true);

    const ydb::Query select_query{R"(
    --!syntax_v1

    SELECT value_int, key, value_str
    FROM test_table
    WHERE key = "key1"
    ORDER BY key;
  )"};
    auto result = GetTableClient().ExecuteDataQuery(select_query);
    ASSERT_EQ(result.GetSingleCursor().AsOptionalSingleRow<tests::RowValue>().value(), kPreFilledRows[0]);
}

UTEST_F(YdbStructFromRow, AsOptionalSingleRowEmpty) {
    CreateTable("test_table", false);

    const ydb::Query select_query{R"(
    --!syntax_v1

    SELECT value_int, key, value_str
    FROM test_table
    WHERE FALSE;
  )"};
    auto result = GetTableClient().ExecuteDataQuery(select_query);
    ASSERT_EQ(result.GetSingleCursor().AsOptionalSingleRow<tests::RowValue>(), std::nullopt);
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
inline constexpr auto ydb::kStructMemberNames<tests::StructReadRowMissingColumn> = ydb::StructMemberNames{};

UTEST_F(YdbStructFromRow, StructReadRowMissingColumn) {
    CreateTable("test_table", true);

    const ydb::Query select_query{R"(
    --!syntax_v1

    SELECT key, value_str, value_int
    FROM test_table
    ORDER BY key;
  )"};
    auto cursor = GetTableClient().ExecuteDataQuery(select_query).GetSingleCursor();
    ASSERT_FALSE(cursor.IsTruncated());

    ASSERT_EQ(cursor.size(), 3);
    UEXPECT_THROW_MSG(
        cursor.GetFirstRow().As<tests::StructReadRowMissingColumn>(),
        ydb::ParseError,
        "Missing column 'extra_value'"
    );
}

namespace tests {

struct StructReadRowExtraColumn {
    std::string key;
    std::string value_str;
};

}  // namespace tests

template <>
inline constexpr auto ydb::kStructMemberNames<tests::StructReadRowExtraColumn> = ydb::StructMemberNames{};

UTEST_F(YdbStructFromRow, StructReadRowExtraColumn) {
    CreateTable("test_table", true);

    const ydb::Query select_query{R"(
    --!syntax_v1

    SELECT key, value_str, value_int
    FROM test_table
    ORDER BY key;
  )"};
    auto cursor = GetTableClient().ExecuteDataQuery(select_query).GetSingleCursor();
    ASSERT_FALSE(cursor.IsTruncated());

    ASSERT_EQ(cursor.size(), 3);
    UEXPECT_THROW_MSG(
        cursor.GetFirstRow().As<tests::StructReadRowExtraColumn>(),
        ydb::ParseError,
        "Unexpected extra columns"
    );
}

USERVER_NAMESPACE_END
