#include <utility>
#include <vector>

#include <boost/pfr/ops_fields.hpp>

#include <userver/utest/utest.hpp>
#include <userver/utils/enumerate.hpp>
#include <userver/ydb/io/list.hpp>
#include <userver/ydb/io/primitives.hpp>
#include <userver/ydb/io/structs.hpp>
#include <userver/ydb/types.hpp>

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

    const ydb::Query select_query{R"(
    --!syntax_v1

    SELECT <|key: key, value_str: value_str, value_int: value_int|> as item
    FROM test_table
    ORDER BY test_table.item.key;
  )"};
    auto cursor = GetTableClient().ExecuteDataQuery(select_query).GetSingleCursor();
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
            .key = "key1",
            .value_str = "value1",
            .value_int = 1,
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
            .key = "key1",
            .value_str = "value1",
            .value_int = 1,
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
            .custom_key = "key1",
            .custom_value_str = "value1",
            .value_int = 1,
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

namespace struct_test {

struct CampaignInput {
    static constexpr ydb::StructMemberNames kYdbMemberNames{};

    ydb::Utf8 current_place_id;
    std::vector<ydb::Utf8> user_id_list;
    std::vector<ydb::Utf8> user_tag_list;
    std::vector<ydb::Utf8> place_id_list;
    std::vector<ydb::Utf8> brand_id_list;

    bool operator==(const CampaignInput&) const = default;
};

struct FlattenedCampaignInput {
    static constexpr ydb::StructMemberNames kYdbMemberNames{};

    ydb::Utf8 current_place_id;
    ydb::Utf8 user_id;
    ydb::Utf8 user_tag;
    ydb::Utf8 place_id;
    ydb::Utf8 brand_id;

    bool operator==(const FlattenedCampaignInput&) const = default;
};

}  // namespace struct_test

UTEST_F(YdbStructIO, ListOfStructsWithUtf8Lists) {
    const ydb::Query query{R"(
        DECLARE $input AS List<Struct<
            current_place_id: Utf8,
            user_id_list: List<Utf8>,
            user_tag_list: List<Utf8>,
            place_id_list: List<Utf8>,
            brand_id_list: List<Utf8>
        >>;
        SELECT $input AS input;
    )"};

    const struct_test::CampaignInput empty_lists{
        .current_place_id = ydb::Utf8{"empty"},
        .user_id_list = {},
        .user_tag_list = {},
        .place_id_list = {},
        .brand_id_list = {},
    };
    const struct_test::CampaignInput filled_lists{
        .current_place_id = ydb::Utf8{"current"},
        .user_id_list = {ydb::Utf8{"uid1"}, ydb::Utf8{"uid2"}},
        .user_tag_list = {ydb::Utf8{"Schlüsselwort"}},
        .place_id_list = {ydb::Utf8{"place1"}, ydb::Utf8{"place2"}},
        .brand_id_list = {ydb::Utf8{"Marke"}},
    };

    const std::vector<std::vector<struct_test::CampaignInput>> inputs{
        {},
        {empty_lists},
        {empty_lists, filled_lists, empty_lists},
    };
    for (const auto& [index, input] : utils::enumerate(inputs)) {
        SCOPED_TRACE(index);
        auto result = GetTableClient().ExecuteDataQuery(ydb::OperationSettings{}, query, "$input", input);
        auto cursor = result.GetSingleCursor();
        ASSERT_FALSE(cursor.IsTruncated());
        ASSERT_EQ(cursor.size(), 1);
        EXPECT_EQ(cursor.GetFirstRow().Get<std::vector<struct_test::CampaignInput>>("input"), input);
    }
}

UTEST_F(YdbStructIO, FlattenListOfStructsWithUtf8Lists) {
    const ydb::Query query{R"(
        DECLARE $input AS List<Struct<
            current_place_id: Utf8,
            user_id_list: List<Utf8>,
            user_tag_list: List<Utf8>,
            place_id_list: List<Utf8>,
            brand_id_list: List<Utf8>
        >>;
        SELECT current_place_id, user_id, user_tag, place_id, brand_id
        FROM AS_TABLE($input)
        FLATTEN LIST BY (
            user_id_list AS user_id,
            user_tag_list AS user_tag,
            place_id_list AS place_id,
            brand_id_list AS brand_id
        )
        ORDER BY current_place_id, user_id, user_tag, place_id, brand_id;
    )"};

    const std::vector input{struct_test::CampaignInput{
        .current_place_id = ydb::Utf8{"current"},
        .user_id_list = {ydb::Utf8{"uid1"}, ydb::Utf8{"uid2"}},
        .user_tag_list = {ydb::Utf8{"Schlüsselwort"}},
        .place_id_list = {ydb::Utf8{"place"}},
        .brand_id_list = {ydb::Utf8{"brand1"}, ydb::Utf8{"brand2"}},
    }};
    const std::vector<struct_test::FlattenedCampaignInput> expected{
        {ydb::Utf8{"current"}, ydb::Utf8{"uid1"}, ydb::Utf8{"Schlüsselwort"}, ydb::Utf8{"place"}, ydb::Utf8{"brand1"}},
        {ydb::Utf8{"current"}, ydb::Utf8{"uid1"}, ydb::Utf8{"Schlüsselwort"}, ydb::Utf8{"place"}, ydb::Utf8{"brand2"}},
        {ydb::Utf8{"current"}, ydb::Utf8{"uid2"}, ydb::Utf8{"Schlüsselwort"}, ydb::Utf8{"place"}, ydb::Utf8{"brand1"}},
        {ydb::Utf8{"current"}, ydb::Utf8{"uid2"}, ydb::Utf8{"Schlüsselwort"}, ydb::Utf8{"place"}, ydb::Utf8{"brand2"}},
    };

    auto result = GetTableClient().ExecuteDataQuery(ydb::OperationSettings{}, query, "$input", input);
    auto cursor = result.GetSingleCursor();
    ASSERT_FALSE(cursor.IsTruncated());
    EXPECT_EQ(std::move(cursor).AsContainer<std::vector<struct_test::FlattenedCampaignInput>>(), expected);
}

USERVER_NAMESPACE_END
