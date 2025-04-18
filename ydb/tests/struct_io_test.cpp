#include <boost/pfr/ops_fields.hpp>

#include <userver/utest/utest.hpp>

#include "small_table.hpp"
#include "test_utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

class YdbStructIO : public YdbSmallTableTest {};

const ydb::Query kInsertQuery{R"(
  --!syntax_v1

  DECLARE $items AS List<Struct<'key': String, 'value_str': String, 'value_int': Int32>>;

  UPSERT INTO test_table
  SELECT * FROM AS_TABLE($items);
)"};

}  // namespace

UTEST_F(YdbStructIO, StructRead) {
    CreateTable("test_table", true);

    const ydb::Query kSelectQuery{R"(
    --!syntax_v1

    SELECT <|key: key, value_str: value_str, value_int: value_int|> as item
    FROM test_table
    ORDER BY test_table.item.key;
  )"};
    auto cursor = GetTableClient().ExecuteDataQuery(kSelectQuery).GetSingleCursor();
    ASSERT_FALSE(cursor.IsTruncated());

    EXPECT_EQ(cursor.size(), 3);
    for (auto [index, row] : utils::enumerate(cursor)) {
        const auto item = row.Get<tests::RowValue>("item");
        EXPECT_TRUE(boost::pfr::eq_fields(item, kPreFilledRows[index]));
    }
}

UTEST_F(YdbStructIO, StructWrite) {
    CreateTable("test_table", false);

    const auto response = GetTableClient().ExecuteDataQuery(
        ydb::OperationSettings{},
        kInsertQuery,
        "$items",
        std::vector{tests::RowValue{
            /*key=*/"key1",
            /*value_str=*/"value1",
            /*value_int=*/1,
        }}
    );
    ASSERT_EQ(response.GetCursorCount(), 0);

    auto result = GetTableClient().ExecuteDataQuery(kSelectAllRows);
    AssertArePreFilledRows(result.GetSingleCursor(), {1});
}

/// [external specialization]
namespace struct_test {

struct MyStructExternalSpecialization {
    std::string key;
    std::string value_str;
    std::int32_t value_int;

    bool operator==(const MyStructExternalSpecialization&) const = default;
};

}  // namespace struct_test

template <>
inline constexpr auto ydb::kStructMemberNames<struct_test::MyStructExternalSpecialization> = ydb::StructMemberNames{};
/// [external specialization]

UTEST_F(YdbStructIO, StructWriteExternalSpecialization) {
    CreateTable("test_table", false);

    const auto response = GetTableClient().ExecuteDataQuery(
        ydb::OperationSettings{},
        kInsertQuery,
        "$items",
        std::vector{struct_test::MyStructExternalSpecialization{
            /*key=*/"key1",
            /*value_str=*/"value1",
            /*value_int=*/1,
        }}
    );
    ASSERT_EQ(response.GetCursorCount(), 0);

    auto result = GetTableClient().ExecuteDataQuery(kSelectAllRows);
    AssertArePreFilledRows(result.GetSingleCursor(), {1});
}

/// [custom names]
namespace struct_test {

struct MyStructCustomNames {
    static constexpr ydb::StructMemberNames kYdbMemberNames{{
        {"custom_key", "key"},
        {"custom_value_str", "value_str"},
    }};

    std::string custom_key;
    std::string custom_value_str;
    std::int32_t value_int;
};

}  // namespace struct_test
/// [custom names]

UTEST_F(YdbStructIO, StructWriteCustomNames) {
    CreateTable("test_table", false);

    const auto response = GetTableClient().ExecuteDataQuery(
        ydb::OperationSettings{},
        kInsertQuery,
        "$items",
        std::vector{struct_test::MyStructCustomNames{
            /*custom_key=*/"key1",
            /*custom_value_str=*/"value1",
            /*value_int=*/1,
        }}
    );
    ASSERT_EQ(response.GetCursorCount(), 0);

    auto result = GetTableClient().ExecuteDataQuery(kSelectAllRows);
    AssertArePreFilledRows(result.GetSingleCursor(), {1});
}

UTEST_F(YdbStructIO, BulkUpsert) {
    CreateTable("test_table", false);

    NYdb::TValueBuilder builder;
    builder.BeginList();
    for (const auto& row_struct : kPreFilledRows) {
        builder.AddListItem();
        ydb::Write(builder, row_struct);
    }
    builder.EndList();

    GetTableClient().BulkUpsert("test_table", builder.Build());

    auto result = GetTableClient().ExecuteDataQuery(kSelectAllRows);
    AssertArePreFilledRows(result.GetSingleCursor(), {1, 2, 3});
}

UTEST_F(YdbStructIO, WriteAndParseOptional) {
    const ydb::Query query{R"(
        --!syntax_v1
        DECLARE $struct_opt AS Struct<'key': String, 'value_str': String, 'value_int': Int32>?;
        SELECT $struct_opt;
    )"};
    const auto struct_opt_value = std::make_optional(struct_test::MyStructExternalSpecialization{
        .key = "key1",
        .value_str = "value1",
        .value_int = 1,
    });
    auto builder = GetTableClient().GetBuilder();
    builder.Add("$struct_opt", struct_opt_value);
    auto result = GetTableClient().ExecuteDataQuery({}, query, std::move(builder));
    for (auto row : result.GetSingleCursor()) {
        EXPECT_EQ(row.Get<std::optional<struct_test::MyStructExternalSpecialization>>(0), struct_opt_value);
    }
}

USERVER_NAMESPACE_END
