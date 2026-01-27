#include <gtest/gtest.h>

#include <limits>
#include <ostream>
#include <string>

#include <fmt/format.h>

#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

constexpr float kMax = std::numeric_limits<float>::max();
constexpr float kMin = std::numeric_limits<float>::min();

struct FloatFromJsonSuccessTestParam {
    std::string input = {};
    FloatMessageData expected_message = {};
    ParseOptions options = {};
};

struct FloatFromJsonFailureTestParam {
    std::string input = {};
    ParseErrorCode expected_errc = {};
    std::string expected_path = {};
    ParseOptions options = {};

    // Native implementation does not emit error on some expected cases which we want to detect.
    // This variable turns off checks for native implementation.
    bool skip_native_check = false;
};

void PrintTo(const FloatFromJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

void PrintTo(const FloatFromJsonFailureTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

class FloatFromJsonSuccessTest : public ::testing::TestWithParam<FloatFromJsonSuccessTestParam> {};
class FloatFromJsonFailureTest : public ::testing::TestWithParam<FloatFromJsonFailureTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    FloatFromJsonSuccessTest,
    ::testing::Values(
        FloatFromJsonSuccessTestParam{R"({})", FloatMessageData{0}},
        FloatFromJsonSuccessTestParam{R"({"field1":null})", FloatMessageData{0}},
        FloatFromJsonSuccessTestParam{R"({"field1":-0.0e0})", FloatMessageData{0}},
        FloatFromJsonSuccessTestParam{R"({"field1":"-0.0e0"})", FloatMessageData{0}},
        FloatFromJsonSuccessTestParam{R"({"field1":123})", FloatMessageData{123}},
        FloatFromJsonSuccessTestParam{R"({"field1":"-123"})", FloatMessageData{-123}},
        FloatFromJsonSuccessTestParam{R"({"field1":100.123567})", FloatMessageData{100.123567}},
        FloatFromJsonSuccessTestParam{R"({"field1":"100.123567"})", FloatMessageData{100.123567}},
        FloatFromJsonSuccessTestParam{R"({"field1":-100.123567})", FloatMessageData{-100.123567}},
        FloatFromJsonSuccessTestParam{R"({"field1":"-100.123567"})", FloatMessageData{-100.123567}},
        FloatFromJsonSuccessTestParam{R"({"field1":100.123e-2})", FloatMessageData{1.00123}},
        FloatFromJsonSuccessTestParam{R"({"field1":"-100.123567E2"})", FloatMessageData{-10012.3567}},
        FloatFromJsonSuccessTestParam{R"({"field1":100.123e-2})", FloatMessageData{1.00123}},
        FloatFromJsonSuccessTestParam{R"({"field1":"-100.123567E2"})", FloatMessageData{-10012.3567}},
        FloatFromJsonSuccessTestParam{R"({"field1":3.40282347e+38})", FloatMessageData{kMax}},
        FloatFromJsonSuccessTestParam{R"({"field1":"3.40282347E38"})", FloatMessageData{kMax}},
        FloatFromJsonSuccessTestParam{R"({"field1":-3.40282347E+38})", FloatMessageData{-kMax}},
        FloatFromJsonSuccessTestParam{R"({"field1":"-3.40282347e38"})", FloatMessageData{-kMax}},
        FloatFromJsonSuccessTestParam{R"({"field1":1.17549435e-38})", FloatMessageData{kMin}},
        FloatFromJsonSuccessTestParam{R"({"field1":"1.17549435E-38"})", FloatMessageData{kMin}},
        FloatFromJsonSuccessTestParam{R"({"field1":-1.17549435E-38})", FloatMessageData{-kMin}},
        FloatFromJsonSuccessTestParam{R"({"field1":"-1.17549435e-38"})", FloatMessageData{-kMin}}
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    FloatFromJsonFailureTest,
    ::testing::Values(
        FloatFromJsonFailureTestParam{R"({"field1":[]})", ParseErrorCode::kInvalidType, "field1"},
        FloatFromJsonFailureTestParam{R"({"field1":{}})", ParseErrorCode::kInvalidType, "field1"},
        FloatFromJsonFailureTestParam{R"({"field1":true})", ParseErrorCode::kInvalidType, "field1"},
        FloatFromJsonFailureTestParam{R"({"field1":3.403e+38})", ParseErrorCode::kInvalidValue, "field1"},
        FloatFromJsonFailureTestParam{R"({"field1":"3.403e+38"})", ParseErrorCode::kInvalidValue, "field1"},
        FloatFromJsonFailureTestParam{R"({"field1":-3.403e+38})", ParseErrorCode::kInvalidValue, "field1"},
        FloatFromJsonFailureTestParam{R"({"field1":"-3.403e+38"})", ParseErrorCode::kInvalidValue, "field1"},
        FloatFromJsonFailureTestParam{R"({"field1":" 123"})", ParseErrorCode::kInvalidValue, "field1", {}, true},
        FloatFromJsonFailureTestParam{R"({"field1":"123 "})", ParseErrorCode::kInvalidValue, "field1", {}, true},
        FloatFromJsonFailureTestParam{R"({"field1":"1a3"})", ParseErrorCode::kInvalidValue, "field1"}
    )
);

TEST_P(FloatFromJsonSuccessTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::FloatMessage message, expected_message, sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);
    expected_message = PrepareTestData(param.expected_message);

    message.set_field1(100.001);

    UASSERT_NO_THROW((message = JsonToMessage<proto_json::messages::FloatMessage>(input, param.options)));
    UASSERT_NO_THROW(InitSampleMessage(param.input, sample_message, param.options));

    CheckMessageEqual(message, sample_message);
    CheckMessageEqual(message, expected_message);
}

TEST_P(FloatFromJsonFailureTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::FloatMessage sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);

    EXPECT_PARSE_ERROR(
        (void)JsonToMessage<proto_json::messages::FloatMessage>(input, param.options),
        param.expected_errc,
        param.expected_path
    );

    if (!param.skip_native_check) {
        UEXPECT_THROW(InitSampleMessage(param.input, sample_message, param.options), SampleError);
    }
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
