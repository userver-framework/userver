#include <gtest/gtest.h>

#include <ostream>
#include <string>

#include <fmt/format.h>

#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

struct RepeatedFromJsonSuccessTestParam {
    std::string input = {};
    RepeatedMessageData expected_message = {};
    ParseOptions options = {};
};

struct RepeatedFromJsonFailureTestParam {
    std::string input = {};
    ParseErrorCode expected_errc = {};
    std::string expected_path = {};
    ParseOptions options = {};

    // Protobuf ProtoJSON legacy syntax supports some features which we want to prohibit (because
    // we do not want our clients to use syntax that may break in the newer protobuf versions). This
    // variable is used disable some checks that will fail for legacy syntax.
    bool skip_native_check = false;
};

void PrintTo(const RepeatedFromJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

void PrintTo(const RepeatedFromJsonFailureTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

class RepeatedFromJsonSuccessTest : public ::testing::TestWithParam<RepeatedFromJsonSuccessTestParam> {};
class RepeatedFromJsonFailureTest : public ::testing::TestWithParam<RepeatedFromJsonFailureTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    RepeatedFromJsonSuccessTest,
    ::testing::Values(
        RepeatedFromJsonSuccessTestParam{R"({})", RepeatedMessageData{}},
        RepeatedFromJsonSuccessTestParam{
            R"({
                "field1":[],"field2":[],"field3":[],"field4":[],"field5":[],"field6":[],"field7":[],
                "field8":[],"field9":[],"field10":[],"field11":[],"field12":[]
            })",
            RepeatedMessageData{}
        },
        RepeatedFromJsonSuccessTestParam{
            // Can't use "field4:null" because native legacy parser treats this as a single item array.
            R"({
                "field1":null,"field2":null,"field3":null,"field5":null,"field6":null,"field7":null,"field8":null,
                "field9":null,"field10":null,"field11":null,"field12":null
            })",
            RepeatedMessageData{}
        },
        RepeatedFromJsonSuccessTestParam{
            R"({
                "field1":[100],
                "field2":[{"field1":true}],
                "field3":["123.987s"],
                "field4":[null],
                "field5":[1],
                "field6":["-1"],
                "field7":["1"],
                "field8":["-1.5"],
                "field9":[1.5],
                "field10":[false],
                "field11":["hello"],
                "field12":["TEST_VALUE1"]
            })",
            RepeatedMessageData{
                {100},
                {{true}},
                {{.seconds = 123, .nanos = 987'000'000}},
                {ProtoValue{ProtoNullValue{}}},
                {1},
                {-1},
                {1},
                {-1.5},
                {1.5},
                {false},
                {"hello"},
                {proto_json::messages::RepeatedMessage::TEST_VALUE1}
            }
        },
        RepeatedFromJsonSuccessTestParam{
            R"({
              "field1":[100,0,200],
              "field2":[{"field1":true},{"field1":false}],
              "field3":["123.987s","0s","-987s"]
            })",
            RepeatedMessageData{
                {100, 0, 200},
                {{true}, {false}},
                {{.seconds = 123, .nanos = 987'000'000}, {}, {.seconds = -987}}
            }
        }
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    RepeatedFromJsonFailureTest,
    ::testing::Values(
        RepeatedFromJsonFailureTestParam{R"({"field1":{}})", ParseErrorCode::kInvalidType, "field1"},
        RepeatedFromJsonFailureTestParam{
            R"({"field1":1})",
            ParseErrorCode::kInvalidType,
            "field1",
            {},
            // Legacy implementation treats single value as array of one item.
            true
        },
        RepeatedFromJsonFailureTestParam{R"({"field2":true})", ParseErrorCode::kInvalidType, "field2", {}, true},
        RepeatedFromJsonFailureTestParam{R"({"field3":"test"})", ParseErrorCode::kInvalidType, "field3", {}, true},
        RepeatedFromJsonFailureTestParam{
            R"({"field1":[1, null, 2]})",
            ParseErrorCode::kInvalidValue,
            "field1[1]",
            {},
            // Legacy implementation ignores null as items.
            true
        },
        RepeatedFromJsonFailureTestParam{R"({"field2":[null]})", ParseErrorCode::kInvalidValue, "field2[0]", {}, true},
        RepeatedFromJsonFailureTestParam{
            R"({"field3":["123.100s", "-123.100s", null]})",
            ParseErrorCode::kInvalidValue,
            "field3[2]",
            {},
            // Legacy implementation ignores null as items.
            true
        },
        RepeatedFromJsonFailureTestParam{
            R"({"field1":[[1,2,3]]})",
            ParseErrorCode::kInvalidType,
            "field1[0]",
            {},
            // Legacy implementation flattens array in this case.
            true
        },
        RepeatedFromJsonFailureTestParam{
            R"({"field2":[[{"field1":true}]]})",
            ParseErrorCode::kInvalidType,
            "field2[0]",
            {},
            // Legacy implementation flattens array in this case.
            true
        },
        RepeatedFromJsonFailureTestParam{
            R"({"field2":[{"field1":true},"oops"]})",
            ParseErrorCode::kInvalidType,
            "field2[1]"
        },
        RepeatedFromJsonFailureTestParam{
            R"({"field3":["123.100s", "oops"]})",
            ParseErrorCode::kInvalidValue,
            "field3[1]"
        }
    )
);

TEST_P(RepeatedFromJsonSuccessTest, Test) {
    using Message = proto_json::messages::RepeatedMessage;
    const auto& param = GetParam();

    Message message;
    Message expected_message;
    Message sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);
    expected_message = PrepareTestData(param.expected_message);

    UASSERT_NO_THROW((message = JsonToMessage<Message>(input, param.options)));
    UASSERT_NO_THROW(InitSampleMessage(param.input, sample_message, param.options));

    CheckMessageEqual(message, sample_message);
    CheckMessageEqual(message, expected_message);
}

TEST_P(RepeatedFromJsonFailureTest, Test) {
    using Message = proto_json::messages::RepeatedMessage;
    const auto& param = GetParam();

    Message sample;
    formats::json::Value input = PrepareJsonTestData(param.input);

    EXPECT_PARSE_ERROR((void)JsonToMessage<Message>(input, param.options), param.expected_errc, param.expected_path);

    if (!param.skip_native_check) {
        UEXPECT_THROW(InitSampleMessage(param.input, sample, param.options), SampleError);
    }
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
