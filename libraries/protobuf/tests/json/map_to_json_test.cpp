#include <gtest/gtest.h>

#include <string>

#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

struct MapToJsonSuccessTestParam {
    MapMessageData input = {};
    std::string expected_json = {};
    PrintOptions options = {};
};

struct MapToJsonFailureTestParam {
    MapMessageData input = {};
    PrintErrorCode expected_errc = {};
    std::string expected_path = {};
    PrintOptions options = {};
};

class MapToJsonSuccessTest : public ::testing::TestWithParam<MapToJsonSuccessTestParam> {};
class MapToJsonFailureTest : public ::testing::TestWithParam<MapToJsonFailureTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    MapToJsonSuccessTest,
    ::testing::Values(
        MapToJsonSuccessTestParam{MapMessageData{}, R"({})"},
        MapToJsonSuccessTestParam{
            MapMessageData{},
            R"({"field1":{},"field2":{},"field3":{},"field4":{},"field5":{},"field6":{},"field7":{}})",
            {.always_print_fields_with_no_presence = true}
        },
        MapToJsonSuccessTestParam{
            MapMessageData{
                .field1 = {{1, 2}, {3, 4}, {5, 6}},
                .field2 = {{10, 1.5}, {20, 1.5}},
                .field3 = {{100, true}, {200, false}},
                .field4 = {{1000, "hello"}, {2000, "world"}},
                .field5 =
                    {{true, proto_json::messages::MapMessage::TEST_ENUM_VALUE1},
                     {false, proto_json::messages::MapMessage::TEST_ENUM_VALUE0}},
                .field6 = {{"aaa", {true}}, {"bbb", {false}}},
                .field7 = {{"ccc", {.seconds = 10, .nanos = 999}}}
            },
            R"({
              "field1":{"1":2,"3":4,"5":6},
              "field2":{"10":1.5,"20":1.5},
              "field3":{"100":true,"200":false},
              "field4":{"1000":"hello","2000":"world"},
              "field5":{"true":"TEST_ENUM_VALUE1","false":"TEST_ENUM_VALUE0"},
              "field6":{"aaa":{"field1":true},"bbb":{}},
              "field7":{"ccc":"10.000000999s"}
            })"
        },
        MapToJsonSuccessTestParam{
            MapMessageData{
                .field5 =
                    {{true, proto_json::messages::MapMessage::TEST_ENUM_VALUE0},
                     {false, proto_json::messages::MapMessage::TEST_ENUM_VALUE1}}
            },
            R"({"field5":{"true":0,"false":1}})",
            {.always_print_enums_as_ints = true}
        }
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    MapToJsonFailureTest,
    ::testing::Values(MapToJsonFailureTestParam{
        MapMessageData{.field7 = {{"aaa", {.seconds = 1, .nanos = -1}}}},
        PrintErrorCode::kInvalidValue,
        "field7['aaa']"
    })
);

TEST_P(MapToJsonSuccessTest, Test) {
    const auto& param = GetParam();

    auto input = PrepareTestData(param.input);
    formats::json::Value json, expected_json, sample_json;

    UASSERT_NO_THROW((json = MessageToJson(input, param.options)));
    UASSERT_NO_THROW((expected_json = PrepareJsonTestData(param.expected_json)));
    UASSERT_NO_THROW((sample_json = CreateSampleJson(input, param.options)));

    EXPECT_EQ(json, expected_json);
    EXPECT_EQ(expected_json, sample_json);
}

TEST_P(MapToJsonFailureTest, Test) {
    const auto& param = GetParam();
    auto input = PrepareTestData(param.input);

    EXPECT_PRINT_ERROR((void)MessageToJson(input, param.options), param.expected_errc, param.expected_path);
    UEXPECT_THROW((void)CreateSampleJson(input, param.options), SampleError);
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
