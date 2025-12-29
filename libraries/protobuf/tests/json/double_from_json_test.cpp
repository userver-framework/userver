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

constexpr double kMax = std::numeric_limits<double>::max();
constexpr double kMin = std::numeric_limits<double>::min();

struct DoubleFromJsonSuccessTestParam {
    std::string input = {};
    DoubleMessageData expected_message = {};
    ReadOptions options = {};
};

struct DoubleFromJsonFailureTestParam {
    std::string input = {};
    ReadErrorCode expected_errc = {};
    std::string expected_path = {};
    ReadOptions options = {};

    // Native implementation does not emit error on some cases which we want to detect.
    // This variable turns off checks for native implementation.
    bool skip_native_check = false;
};

void PrintTo(const DoubleFromJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

void PrintTo(const DoubleFromJsonFailureTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

class DoubleFromJsonSuccessTest : public ::testing::TestWithParam<DoubleFromJsonSuccessTestParam> {};
class DoubleFromJsonFailureTest : public ::testing::TestWithParam<DoubleFromJsonFailureTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    DoubleFromJsonSuccessTest,
    ::testing::Values(
        DoubleFromJsonSuccessTestParam{R"({})", DoubleMessageData{0}},
        DoubleFromJsonSuccessTestParam{R"({"field1":null})", DoubleMessageData{0}},
        DoubleFromJsonSuccessTestParam{R"({"field1":-0.0e0})", DoubleMessageData{0}},
        DoubleFromJsonSuccessTestParam{R"({"field1":"-0.0e0"})", DoubleMessageData{0}},
        DoubleFromJsonSuccessTestParam{R"({"field1":123})", DoubleMessageData{123}},
        DoubleFromJsonSuccessTestParam{R"({"field1":"-123"})", DoubleMessageData{-123}},
        DoubleFromJsonSuccessTestParam{R"({"field1":100.123567})", DoubleMessageData{100.123567}},
        DoubleFromJsonSuccessTestParam{R"({"field1":"100.123567"})", DoubleMessageData{100.123567}},
        DoubleFromJsonSuccessTestParam{R"({"field1":-100.123567})", DoubleMessageData{-100.123567}},
        DoubleFromJsonSuccessTestParam{R"({"field1":"-100.123567"})", DoubleMessageData{-100.123567}},
        DoubleFromJsonSuccessTestParam{R"({"field1":100.123e-2})", DoubleMessageData{1.00123}},
        DoubleFromJsonSuccessTestParam{R"({"field1":"-100.123567E2"})", DoubleMessageData{-10012.3567}},
        DoubleFromJsonSuccessTestParam{R"({"field1":100.123e-2})", DoubleMessageData{1.00123}},
        DoubleFromJsonSuccessTestParam{R"({"field1":"-100.123567E2"})", DoubleMessageData{-10012.3567}},
        DoubleFromJsonSuccessTestParam{R"({"field1":1.7976931348623157e+308})", DoubleMessageData{kMax}},
        DoubleFromJsonSuccessTestParam{R"({"field1":"1.7976931348623157E308"})", DoubleMessageData{kMax}},
        DoubleFromJsonSuccessTestParam{R"({"field1":-1.7976931348623157E+308})", DoubleMessageData{-kMax}},
        DoubleFromJsonSuccessTestParam{R"({"field1":"-1.7976931348623157e308"})", DoubleMessageData{-kMax}},
        DoubleFromJsonSuccessTestParam{R"({"field1":2.2250738585072014e-308})", DoubleMessageData{kMin}},
        DoubleFromJsonSuccessTestParam{R"({"field1":"2.2250738585072014E-308"})", DoubleMessageData{kMin}},
        DoubleFromJsonSuccessTestParam{R"({"field1":-2.2250738585072014E-308})", DoubleMessageData{-kMin}},
        DoubleFromJsonSuccessTestParam{R"({"field1":"-2.2250738585072014e-308"})", DoubleMessageData{-kMin}}
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    DoubleFromJsonFailureTest,
    ::testing::Values(
        DoubleFromJsonFailureTestParam{R"({"field1":[]})", ReadErrorCode::kInvalidType, "field1"},
        DoubleFromJsonFailureTestParam{R"({"field1":{}})", ReadErrorCode::kInvalidType, "field1"},
        DoubleFromJsonFailureTestParam{R"({"field1":true})", ReadErrorCode::kInvalidType, "field1"},
        DoubleFromJsonFailureTestParam{R"({"field1":" 123"})", ReadErrorCode::kInvalidValue, "field1", {}, true},
        DoubleFromJsonFailureTestParam{R"({"field1":"123 "})", ReadErrorCode::kInvalidValue, "field1", {}, true},
        DoubleFromJsonFailureTestParam{R"({"field1":"1a3"})", ReadErrorCode::kInvalidValue, "field1"}
    )
);

TEST_P(DoubleFromJsonSuccessTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::DoubleMessage message, expected_message, sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);
    expected_message = PrepareTestData(param.expected_message);

    message.set_field1(100.001);

    UASSERT_NO_THROW((message = JsonToMessage<proto_json::messages::DoubleMessage>(input, param.options)));
    UASSERT_NO_THROW(InitSampleMessage(param.input, param.options, sample_message));

    CheckMessageEqual(message, sample_message);
    CheckMessageEqual(message, expected_message);
}

TEST_P(DoubleFromJsonFailureTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::DoubleMessage sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);

    EXPECT_READ_ERROR(
        (void)JsonToMessage<proto_json::messages::DoubleMessage>(input, param.options),
        param.expected_errc,
        param.expected_path
    );

    if (!param.skip_native_check) {
        UEXPECT_THROW(InitSampleMessage(param.input, param.options, sample_message), SampleError);
    }
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
