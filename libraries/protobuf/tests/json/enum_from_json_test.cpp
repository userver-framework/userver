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

namespace msgs = proto_json::messages;

constexpr std::int32_t kMax = std::numeric_limits<std::int32_t>::max();  // 2147483647
constexpr std::int32_t kMin = std::numeric_limits<std::int32_t>::min();  // -2147483648

struct EnumFromJsonSuccessTestParam {
    std::string input = {};
    EnumMessageData expected_message = {};
    ParseOptions options = {};
};

struct EnumFromJsonFailureTestParam {
    std::string input = {};
    ParseErrorCode expected_errc = {};
    std::string expected_path = {};
    ParseOptions options = {};
};

void PrintTo(const EnumFromJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

void PrintTo(const EnumFromJsonFailureTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

class EnumFromJsonSuccessTest : public ::testing::TestWithParam<EnumFromJsonSuccessTestParam> {};
class EnumFromJsonFailureTest : public ::testing::TestWithParam<EnumFromJsonFailureTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    EnumFromJsonSuccessTest,
    ::testing::Values(
        EnumFromJsonSuccessTestParam{R"({})", EnumMessageData{msgs::EnumMessage::TEST_UNSPECIFIED}},
        EnumFromJsonSuccessTestParam{R"({"field1":null})", EnumMessageData{msgs::EnumMessage::TEST_UNSPECIFIED}},
        EnumFromJsonSuccessTestParam{
            R"({"field1":"TEST_UNSPECIFIED"})",
            EnumMessageData{msgs::EnumMessage::TEST_UNSPECIFIED}
        },
        EnumFromJsonSuccessTestParam{R"({"field1":"TEST_VALUE1"})", EnumMessageData{msgs::EnumMessage::TEST_VALUE1}},
        EnumFromJsonSuccessTestParam{R"({"field1":"TEST_VALUE2"})", EnumMessageData{msgs::EnumMessage::TEST_VALUE2}},
        EnumFromJsonSuccessTestParam{
            R"({"field1":"TEST_VALUE2_ALIAS"})",
            EnumMessageData{msgs::EnumMessage::TEST_VALUE2}
        },
        EnumFromJsonSuccessTestParam{R"({"field1":"TEST_VALUE3"})", EnumMessageData{msgs::EnumMessage::TEST_VALUE3}},
        EnumFromJsonSuccessTestParam{R"({"field1":0})", EnumMessageData{msgs::EnumMessage::TEST_UNSPECIFIED}},
        EnumFromJsonSuccessTestParam{R"({"field1":1})", EnumMessageData{msgs::EnumMessage::TEST_VALUE1}},
        EnumFromJsonSuccessTestParam{R"({"field1":2})", EnumMessageData{msgs::EnumMessage::TEST_VALUE2}},
        EnumFromJsonSuccessTestParam{R"({"field1":3})", EnumMessageData{msgs::EnumMessage::TEST_VALUE3}},
        EnumFromJsonSuccessTestParam{R"({"field1":5})", EnumMessageData{static_cast<msgs::EnumMessage::Test>(5)}},
        EnumFromJsonSuccessTestParam{R"({"field1":-5})", EnumMessageData{static_cast<msgs::EnumMessage::Test>(-5)}},
        EnumFromJsonSuccessTestParam{
            R"({"field1":2147483647})",
            EnumMessageData{static_cast<msgs::EnumMessage::Test>(kMax)}
        },
        EnumFromJsonSuccessTestParam{
            R"({"field1":-2147483648})",
            EnumMessageData{static_cast<msgs::EnumMessage::Test>(kMin)}
        }
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    EnumFromJsonFailureTest,
    ::testing::Values(
        EnumFromJsonFailureTestParam{R"({"field1":[]})", ParseErrorCode::kInvalidType, "field1"},
        EnumFromJsonFailureTestParam{R"({"field1":{}})", ParseErrorCode::kInvalidType, "field1"},
        EnumFromJsonFailureTestParam{R"({"field1":true})", ParseErrorCode::kInvalidType, "field1"},
        EnumFromJsonFailureTestParam{R"({"field1":"TEST_NON_EXISTENT"})", ParseErrorCode::kUnknownEnum, "field1"},
        EnumFromJsonFailureTestParam{R"({"field1":2147483648})", ParseErrorCode::kInvalidValue, "field1"},
        EnumFromJsonFailureTestParam{R"({"field1":-2147483649})", ParseErrorCode::kInvalidValue, "field1"}
    )
);

TEST_P(EnumFromJsonSuccessTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::EnumMessage message, expected_message, sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);
    expected_message = PrepareTestData(param.expected_message);

    message.set_field1(static_cast<proto_json::messages::EnumMessage::Test>(100001));

    UASSERT_NO_THROW((message = JsonToMessage<proto_json::messages::EnumMessage>(input, param.options)));
    UASSERT_NO_THROW(InitSampleMessage(param.input, sample_message, param.options));

    CheckMessageEqual(message, sample_message);
    CheckMessageEqual(message, expected_message);
}

TEST_P(EnumFromJsonFailureTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::EnumMessage sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);

    EXPECT_PARSE_ERROR(
        (void)JsonToMessage<proto_json::messages::EnumMessage>(input, param.options),
        param.expected_errc,
        param.expected_path
    );

    UEXPECT_THROW(InitSampleMessage(param.input, sample_message, param.options), SampleError);
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
