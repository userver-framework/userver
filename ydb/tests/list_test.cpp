#include <ranges>
#include <unordered_set>
#include <vector>

#include <gmock/gmock.h>
#include <ydb-cpp-sdk/client/value/value.h>

#include <userver/utest/utest.hpp>
#include <userver/ydb/io/list.hpp>
#include <userver/ydb/io/primitives.hpp>

#include "small_table.hpp"
#include "test_utils.hpp"

USERVER_NAMESPACE_BEGIN

static_assert(std::input_iterator<ydb::impl::ParseItemsIterator<std::int32_t>>);

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

UTEST_F(YdbListIO, ParseItemsIteratorPostfixIncrement) {
    NYdb::TValueBuilder builder;
    builder.BeginList();
    builder.AddListItem().Int32(1);
    builder.AddListItem().Int32(2);
    builder.AddListItem().Int32(3);
    builder.EndList();

    NYdb::TValueParser parser{builder.Build()};
    const ydb::ParseContext context{.column_name = "list"};

    parser.OpenList();
    ydb::impl::ParseItemsIterator<std::int32_t> it{parser, context};
    const ydb::impl::ParseItemsIterator<std::int32_t> end{};

    ASSERT_NE(it, end);
    EXPECT_EQ(*it, 1);
    it++;
    ASSERT_NE(it, end);
    EXPECT_EQ(*it, 2);
    it++;
    ASSERT_NE(it, end);
    EXPECT_EQ(*it, 3);
    it++;
    EXPECT_EQ(it, end);
    parser.CloseList();
}

UTEST_F(YdbListIO, ReadUnorderedSet) {
    auto response = GetTableClient().ExecuteDataQuery(R"(
    SELECT AsList(1, 2, 3, 4, 5);
  )");

    auto cursor = response.GetSingleCursor();
    const auto list = cursor.GetFirstRow().Get<std::unordered_set<std::int32_t>>(0);
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

    auto serialized_rows = std::views::iota(0, 3) | std::views::transform([](int i) { return kPreFilledRows[i]; });
    GetTableClient().BulkUpsert("test_table", std::move(serialized_rows));

    auto result = GetTableClient().ExecuteDataQuery(kSelectAllRows);
    AssertArePreFilledRows(result.GetSingleCursor(), {1, 2, 3});
}

UTEST_F(YdbListIO, BulkUpsertEmptyRangeOfStructs) {
    CreateTable("test_table", false);

    auto serialized_rows = std::views::iota(0, 0) | std::views::transform([](int i) { return kPreFilledRows[i]; });
    GetTableClient().BulkUpsert("test_table", std::move(serialized_rows));

    auto result = GetTableClient().ExecuteDataQuery(kSelectAllRows);
    AssertArePreFilledRows(result.GetSingleCursor(), {});
}

UTEST_F(YdbListIO, WriteAndParseOptional) {
    const ydb::Query query{R"(
        --!syntax_v1
        DECLARE $list_opt AS List<String>?;
        SELECT $list_opt;
    )"};
    const auto list_opt_value = std::make_optional(std::vector<std::string>{"value1", "value2"});
    auto builder = GetTableClient().GetBuilder();
    builder.Add("$list_opt", list_opt_value);
    auto result = GetTableClient().ExecuteDataQuery({}, query, std::move(builder));
    for (auto row : result.GetSingleCursor()) {
        EXPECT_EQ(row.Get<std::optional<std::vector<std::string>>>(0), list_opt_value);
    }
}

USERVER_NAMESPACE_END
