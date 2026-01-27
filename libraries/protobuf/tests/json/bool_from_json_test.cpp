#include <gtest/gtest.h>

#include <ostream>
#include <string>

#include <fmt/format.h>

#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

struct BoolFromJsonSuccessTestParam {
    std::string input = {};
    BoolMessageData expected_message = {};
    ParseOptions options = {};
};

struct BoolFromJsonFailureTestParam {
    std::string input = {};
    ParseErrorCode expected_errc = {};
    std::string expected_path = {};
    ParseOptions options = {};

    // Protobuf ProtoJSON legacy syntax supports out-of-spec "true"/"false" strings as a value
    // for bool, which we want to prohibit (because we do not want our clients to use syntax
    // that may break in the newer protobuf versions). This variable is used disable some checks
    // that will fail for legacy syntax.
    bool skip_native_check = false;
};

void PrintTo(const BoolFromJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

void PrintTo(const BoolFromJsonFailureTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

class BoolFromJsonSuccessTest : public ::testing::TestWithParam<BoolFromJsonSuccessTestParam> {};
class BoolFromJsonFailureTest : public ::testing::TestWithParam<BoolFromJsonFailureTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    BoolFromJsonSuccessTest,
    ::testing::Values(
        BoolFromJsonSuccessTestParam{R"({})", BoolMessageData{false}},
        BoolFromJsonSuccessTestParam{R"({"field1":null})", BoolMessageData{false}},
        BoolFromJsonSuccessTestParam{R"({"field1":false})", BoolMessageData{false}},
        BoolFromJsonSuccessTestParam{R"({"field1":true})", BoolMessageData{true}}
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    BoolFromJsonFailureTest,
    ::testing::Values(
        BoolFromJsonFailureTestParam{R"({"field1":[]})", ParseErrorCode::kInvalidType, "field1"},
        BoolFromJsonFailureTestParam{R"({"field1":{}})", ParseErrorCode::kInvalidType, "field1"},
        BoolFromJsonFailureTestParam{R"({"field1":"false"})", ParseErrorCode::kInvalidType, "field1", {}, true},
        BoolFromJsonFailureTestParam{R"({"field1":"true"})", ParseErrorCode::kInvalidType, "field1", {}, true},
        BoolFromJsonFailureTestParam{R"({"field1":0})", ParseErrorCode::kInvalidType, "field1"},
        BoolFromJsonFailureTestParam{R"({"field1":1})", ParseErrorCode::kInvalidType, "field1"}
    )
);

TEST_P(BoolFromJsonSuccessTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::BoolMessage message, expected_message, sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);
    expected_message = PrepareTestData(param.expected_message);

    UASSERT_NO_THROW((message = JsonToMessage<proto_json::messages::BoolMessage>(input, param.options)));
    UASSERT_NO_THROW(InitSampleMessage(param.input, sample_message, param.options));

    CheckMessageEqual(message, sample_message);
    CheckMessageEqual(message, expected_message);
}

TEST_P(BoolFromJsonFailureTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::BoolMessage sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);

    EXPECT_PARSE_ERROR(
        (void)JsonToMessage<proto_json::messages::BoolMessage>(input, param.options),
        param.expected_errc,
        param.expected_path
    );

    if (!param.skip_native_check) {
        UEXPECT_THROW(InitSampleMessage(param.input, sample_message, param.options), SampleError);
    }
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
