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
    ReadOptions options = {};
};

struct RepeatedFromJsonFailureTestParam {
    std::string input = {};
    ReadErrorCode expected_errc = {};
    std::string expected_path = {};
    ReadOptions options = {};

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
        RepeatedFromJsonSuccessTestParam{R"({"field1":[],"field2":[],"field3":[]})", RepeatedMessageData{}},
        RepeatedFromJsonSuccessTestParam{R"({"field1":null,"field2":null,"field3":null})", RepeatedMessageData{}},
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
        RepeatedFromJsonFailureTestParam{R"({"field1":{}})", ReadErrorCode::kInvalidType, "field1"},
        RepeatedFromJsonFailureTestParam{
            R"({"field1":1})",
            ReadErrorCode::kInvalidType,
            "field1",
            {},
            true  // legacy implementation treats single value as array of one item
        },
        RepeatedFromJsonFailureTestParam{R"({"field2":true})", ReadErrorCode::kInvalidType, "field2", {}, true},
        RepeatedFromJsonFailureTestParam{R"({"field3":"test"})", ReadErrorCode::kInvalidType, "field3", {}, true},
        RepeatedFromJsonFailureTestParam{
            R"({"field1":[1, null, 2]})",
            ReadErrorCode::kInvalidValue,
            "field1[1]",
            {},
            true  // legacy implementation ignores null as items
        },
        RepeatedFromJsonFailureTestParam{R"({"field2":[null]})", ReadErrorCode::kInvalidValue, "field2[0]", {}, true},
        RepeatedFromJsonFailureTestParam{
            R"({"field3":["123.100s", "-123.100s", null]})",
            ReadErrorCode::kInvalidValue,
            "field3[2]",
            {},
            true  // legacy implementation ignores null as items
        },
        RepeatedFromJsonFailureTestParam{
            R"({"field2":[{"field1":true},"oops"]})",
            ReadErrorCode::kInvalidType,
            "field2[1]"
        },
        RepeatedFromJsonFailureTestParam{
            R"({"field3":["123.100s", "oops"]})",
            ReadErrorCode::kInvalidValue,
            "field3[1]"
        }
    )
);

TEST_P(RepeatedFromJsonSuccessTest, Test) {
    using Message = proto_json::messages::RepeatedMessage;
    const auto& param = GetParam();

    Message message, expected_message, sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);
    expected_message = PrepareTestData(param.expected_message);

    UASSERT_NO_THROW((message = JsonToMessage<Message>(input, param.options)));
    UASSERT_NO_THROW(InitSampleMessage(param.input, param.options, sample_message));

    CheckMessageEqual(message, sample_message);
    CheckMessageEqual(message, expected_message);
}

TEST_P(RepeatedFromJsonFailureTest, Test) {
    using Message = proto_json::messages::RepeatedMessage;
    const auto& param = GetParam();

    Message sample;
    formats::json::Value input = PrepareJsonTestData(param.input);

    EXPECT_READ_ERROR((void)JsonToMessage<Message>(input, param.options), param.expected_errc, param.expected_path);

    if (!param.skip_native_check) {
        UEXPECT_THROW(InitSampleMessage(param.input, param.options, sample), SampleError);
    }
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
