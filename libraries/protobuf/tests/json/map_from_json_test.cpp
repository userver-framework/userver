#include <gtest/gtest.h>

#include <ostream>
#include <string>

#include <fmt/format.h>

#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

struct MapFromJsonSuccessTestParam {
    std::string input = {};
    MapMessageData expected_message = {};
    ParseOptions options = {};
};

struct MapFromJsonFailureTestParam {
    std::string input = {};
    ParseErrorCode expected_errc = {};
    std::string expected_path = {};
    ParseOptions options = {};
};

void PrintTo(const MapFromJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

void PrintTo(const MapFromJsonFailureTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

class MapFromJsonSuccessTest : public ::testing::TestWithParam<MapFromJsonSuccessTestParam> {};
class MapFromJsonFailureTest : public ::testing::TestWithParam<MapFromJsonFailureTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    MapFromJsonSuccessTest,
    ::testing::Values(
        MapFromJsonSuccessTestParam{R"({})", MapMessageData{}},
        MapFromJsonSuccessTestParam{
            R"({"field1":{},"field2":{},"field3":{},"field4":{},"field5":{},"field6":{},"field7":{}})",
            MapMessageData{}
        },
        MapFromJsonSuccessTestParam{
            R"({
              "field1":{"1":2,"3":4,"5":6},
              "field2":{"10":1.5,"20":1.5},
              "field3":{"100":true,"200":false},
              "field4":{"1000":"hello","2000":"world"},
              "field5":{"true":"TEST_ENUM_VALUE1","false":"TEST_ENUM_VALUE0"},
              "field6":{"aaa":{"field1":true},"bbb":{}},
              "field7":{"ccc":"10.000000999s"}
            })",
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
            }
        },
        MapFromJsonSuccessTestParam{
            R"({"field5":{"true":0,"false":1}})",
            MapMessageData{
                .field5 =
                    {{true, proto_json::messages::MapMessage::TEST_ENUM_VALUE0},
                     {false, proto_json::messages::MapMessage::TEST_ENUM_VALUE1}}
            }
        }
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    MapFromJsonFailureTest,
    ::testing::Values(
        MapFromJsonFailureTestParam{R"({"field1":[]})", ParseErrorCode::kInvalidType, "field1"},
        MapFromJsonFailureTestParam{R"({"field2":1})", ParseErrorCode::kInvalidType, "field2"},
        MapFromJsonFailureTestParam{R"({"field3":true})", ParseErrorCode::kInvalidType, "field3"},
        MapFromJsonFailureTestParam{R"({"field7":"test"})", ParseErrorCode::kInvalidType, "field7"},
        MapFromJsonFailureTestParam{R"({"field1":{"":1}})", ParseErrorCode::kInvalidValue, "field1.''"},
        MapFromJsonFailureTestParam{
            R"({"field2":{"10":1.5,"-10":0,"0":0}})",
            ParseErrorCode::kInvalidValue,
            "field2.-10"
        },
        MapFromJsonFailureTestParam{
            R"({"field3":{"10":true,"10x":false}})",
            ParseErrorCode::kInvalidValue,
            "field3.10x"
        },
        MapFromJsonFailureTestParam{R"({"field4":{"1.5":"hello"}})", ParseErrorCode::kInvalidValue, "field4.'1.5'"},
        MapFromJsonFailureTestParam{R"({"field5":{"TRUE":0}})", ParseErrorCode::kInvalidValue, "field5.TRUE"},
        MapFromJsonFailureTestParam{R"({"field7":{"aaa":"oops"}})", ParseErrorCode::kInvalidValue, "field7.aaa"}
    )
);

TEST_P(MapFromJsonSuccessTest, Test) {
    using Message = proto_json::messages::MapMessage;
    const auto& param = GetParam();

    Message message, expected_message, sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);
    expected_message = PrepareTestData(param.expected_message);

    UASSERT_NO_THROW((message = JsonToMessage<Message>(input, param.options)));
    UASSERT_NO_THROW(InitSampleMessage(param.input, sample_message, param.options));

    CheckMessageEqual(message, sample_message);
    CheckMessageEqual(message, expected_message);
}

TEST_P(MapFromJsonFailureTest, Test) {
    using Message = proto_json::messages::MapMessage;
    const auto& param = GetParam();

    Message sample;
    formats::json::Value input = PrepareJsonTestData(param.input);

    EXPECT_PARSE_ERROR((void)JsonToMessage<Message>(input, param.options), param.expected_errc, param.expected_path);
    UEXPECT_THROW(InitSampleMessage(param.input, sample, param.options), SampleError);
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
