#include <string>
#include <tuple>

#include <userver/utest/utest.hpp>

#include "test_utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace {
class YdbTupleIO : public ydb::ClientFixtureBase {};
}  // namespace

UTEST_F(YdbTupleIO, ReadTuple) {
    auto response = GetTableClient().ExecuteDataQuery(R"(
        SELECT AsTuple(1, "hello", true);
    )");

    auto cursor = response.GetSingleCursor();
    const auto value = cursor.GetFirstRow().Get<std::tuple<std::int32_t, std::string, bool>>(0);
    EXPECT_EQ(std::get<0>(value), 1);
    EXPECT_EQ(std::get<1>(value), "hello");
    EXPECT_EQ(std::get<2>(value), true);
}

UTEST_F(YdbTupleIO, ReadSingleElementTuple) {
    auto response = GetTableClient().ExecuteDataQuery(R"(
        SELECT AsTuple(42);
    )");

    auto cursor = response.GetSingleCursor();
    const auto value = cursor.GetFirstRow().Get<std::tuple<std::int32_t>>(0);
    EXPECT_EQ(std::get<0>(value), 42);
}

UTEST_F(YdbTupleIO, WriteAndReadTuple) {
    const ydb::Query query{R"(
        --!syntax_v1
        DECLARE $tuple_val AS Tuple<Int32, String, Bool>;
        SELECT $tuple_val;
    )"};

    const auto tuple_value = std::make_tuple(std::int32_t{123}, std::string{"world"}, false);
    auto builder = GetTableClient().GetBuilder();
    builder.Add("$tuple_val", tuple_value);
    auto result = GetTableClient().ExecuteDataQuery({}, query, std::move(builder));
    for (auto row : result.GetSingleCursor()) {
        const auto parsed = row.Get<std::tuple<std::int32_t, std::string, bool>>(0);
        EXPECT_EQ(parsed, tuple_value);
    }
}

UTEST_F(YdbTupleIO, WriteAndReadOptionalTuple) {
    const ydb::Query query{R"(
        --!syntax_v1
        DECLARE $tuple_opt AS Tuple<Int32, String>?;
        SELECT $tuple_opt;
    )"};

    const auto tuple_opt_value = std::make_optional(std::make_tuple(std::int32_t{456}, std::string{"opt"}));
    auto builder = GetTableClient().GetBuilder();
    builder.Add("$tuple_opt", tuple_opt_value);
    auto result = GetTableClient().ExecuteDataQuery({}, query, std::move(builder));
    for (auto row : result.GetSingleCursor()) {
        EXPECT_EQ((row.Get<std::optional<std::tuple<std::int32_t, std::string>>>(0)), tuple_opt_value);
    }
}

UTEST_F(YdbTupleIO, ReadNestedTupleInList) {
    auto response = GetTableClient().ExecuteDataQuery(R"(
        SELECT AsList(AsTuple(1, "a"), AsTuple(2, "b"), AsTuple(3, "c"));
    )");

    auto cursor = response.GetSingleCursor();
    const auto value = cursor.GetFirstRow().Get<std::vector<std::tuple<std::int32_t, std::string>>>(0);
    ASSERT_EQ(value.size(), 3);
    EXPECT_EQ(std::get<0>(value[0]), 1);
    EXPECT_EQ(std::get<1>(value[0]), "a");
    EXPECT_EQ(std::get<0>(value[1]), 2);
    EXPECT_EQ(std::get<1>(value[1]), "b");
    EXPECT_EQ(std::get<0>(value[2]), 3);
    EXPECT_EQ(std::get<1>(value[2]), "c");
}

USERVER_NAMESPACE_END
