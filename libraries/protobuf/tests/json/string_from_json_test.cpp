#include <gtest/gtest.h>

#include <ostream>
#include <string>

#include <fmt/format.h>

#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

struct StringFromJsonSuccessTestParam {
    std::string input = {};
    StringMessageData expected_message = {};
    ParseOptions options = {};
};

struct StringFromJsonFailureTestParam {
    std::string input = {};
    ParseErrorCode expected_errc = {};
    std::string expected_path = {};
    ParseOptions options = {};
};

void PrintTo(const StringFromJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

void PrintTo(const StringFromJsonFailureTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

class StringFromJsonSuccessTest : public ::testing::TestWithParam<StringFromJsonSuccessTestParam> {};
class StringFromJsonFailureTest : public ::testing::TestWithParam<StringFromJsonFailureTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    StringFromJsonSuccessTest,
    ::testing::Values(
        StringFromJsonSuccessTestParam{R"({})", StringMessageData{""}},
        StringFromJsonSuccessTestParam{R"({"field1":null})", StringMessageData{""}},
        StringFromJsonSuccessTestParam{R"({"field1":""})", StringMessageData{""}},
        StringFromJsonSuccessTestParam{R"({"field1":"hello world"})", StringMessageData{"hello world"}}
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    StringFromJsonFailureTest,
    ::testing::Values(
        StringFromJsonFailureTestParam{R"({"field1":[]})", ParseErrorCode::kInvalidType, "field1"},
        StringFromJsonFailureTestParam{R"({"field1":{}})", ParseErrorCode::kInvalidType, "field1"},
        StringFromJsonFailureTestParam{R"({"field1":true})", ParseErrorCode::kInvalidType, "field1"},
        StringFromJsonFailureTestParam{R"({"field1":10})", ParseErrorCode::kInvalidType, "field1"}
    )
);

TEST_P(StringFromJsonSuccessTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::StringMessage message, expected_message, sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);
    expected_message = PrepareTestData(param.expected_message);

    message.set_field1("dump");

    UASSERT_NO_THROW((message = JsonToMessage<proto_json::messages::StringMessage>(input, param.options)));
    UASSERT_NO_THROW(InitSampleMessage(param.input, sample_message, param.options));

    CheckMessageEqual(message, sample_message);
    CheckMessageEqual(message, expected_message);
}

TEST_P(StringFromJsonFailureTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::StringMessage sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);

    EXPECT_PARSE_ERROR(
        (void)JsonToMessage<proto_json::messages::StringMessage>(input, param.options),
        param.expected_errc,
        param.expected_path
    );
    UEXPECT_THROW(InitSampleMessage(param.input, sample_message, param.options), SampleError);
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
