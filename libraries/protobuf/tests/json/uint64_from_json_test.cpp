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

constexpr std::uint64_t kMax = std::numeric_limits<std::uint64_t>::max();  // 18446744073709551615

struct UInt64FromJsonSuccessTestParam {
    std::string input = {};
    UInt64MessageData expected_message = {};
    ParseOptions options = {};

    // Older protobuf libraries does not support exponential notation for the quoted integers.
    // This was fixed in the "v30.0-rc1" / "v6.30.0-rc1" (GOOGLE_PROTOBUF_VERSION = 6030000).
    bool skip_native_parsing_for_older_versions = false;
};

struct UInt64FromJsonFailureTestParam {
    std::string input = {};
    ParseErrorCode expected_errc = {};
    std::string expected_path = {};
    ParseOptions options = {};
};

void PrintTo(const UInt64FromJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

void PrintTo(const UInt64FromJsonFailureTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

class UInt64FromJsonSuccessTest : public ::testing::TestWithParam<UInt64FromJsonSuccessTestParam> {};
class UInt64FromJsonFailureTest : public ::testing::TestWithParam<UInt64FromJsonFailureTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    UInt64FromJsonSuccessTest,
    ::testing::Values(
        UInt64FromJsonSuccessTestParam{R"({})", UInt64MessageData{0, 0}},
        UInt64FromJsonSuccessTestParam{R"({"field1":null,"field2":null})", UInt64MessageData{0, 0}},
        UInt64FromJsonSuccessTestParam{R"({"field1":0,"field2":1})", UInt64MessageData{0, 1}},
        UInt64FromJsonSuccessTestParam{R"({"field1":"0","field2":"1"})", UInt64MessageData{0, 1}},
        UInt64FromJsonSuccessTestParam{
            R"({"field1":18446744073709551615,"field2":"18446744073709551615"})",
            UInt64MessageData{kMax, kMax}
        },
        UInt64FromJsonSuccessTestParam{R"({"field1":10e2,"field2":3e0})", UInt64MessageData{1000, 3}},
        UInt64FromJsonSuccessTestParam{R"({"field1":"10e2","field2":"3e0"})", UInt64MessageData{1000, 3}, {}, true},
        UInt64FromJsonSuccessTestParam{R"({"field1":8500E-2,"field2":1.8E+2})", UInt64MessageData{85, 180}},
        UInt64FromJsonSuccessTestParam{
            R"({"field1":"8500E-2","field2":"1.8E+2"})",
            UInt64MessageData{85, 180},
            {},
            true
        }
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    UInt64FromJsonFailureTest,
    ::testing::Values(
        UInt64FromJsonFailureTestParam{R"({"field1":[],"field2":1})", ParseErrorCode::kInvalidType, "field1"},
        UInt64FromJsonFailureTestParam{R"({"field1":1,"field2":{}})", ParseErrorCode::kInvalidType, "field2"},
        UInt64FromJsonFailureTestParam{R"({"field1":1,"field2":true})", ParseErrorCode::kInvalidType, "field2"},
        UInt64FromJsonFailureTestParam{
            R"({"field1":18446744073709551616,"field2":1})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        UInt64FromJsonFailureTestParam{R"({"field1":1,"field2":-1})", ParseErrorCode::kInvalidValue, "field2"},
        UInt64FromJsonFailureTestParam{
            R"({"field1":"18446744073709551616","field2":1})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        UInt64FromJsonFailureTestParam{R"({"field1":1,"field2":"-1"})", ParseErrorCode::kInvalidValue, "field2"},
        UInt64FromJsonFailureTestParam{R"({"field1":1e50,"field2":1})", ParseErrorCode::kInvalidValue, "field1"},
        UInt64FromJsonFailureTestParam{R"({"field1":1,"field2":-1e50})", ParseErrorCode::kInvalidValue, "field2"},
        UInt64FromJsonFailureTestParam{R"({"field1":" 123","field2":1})", ParseErrorCode::kInvalidValue, "field1"},
        UInt64FromJsonFailureTestParam{R"({"field1":1,"field2":"123 "})", ParseErrorCode::kInvalidValue, "field2"},
        UInt64FromJsonFailureTestParam{R"({"field1":"1a3","field2":1})", ParseErrorCode::kInvalidValue, "field1"},
        UInt64FromJsonFailureTestParam{R"({"field1":"1.1","field2":1})", ParseErrorCode::kInvalidValue, "field1"},
        UInt64FromJsonFailureTestParam{R"({"field1":1,"field2":"-1e50"})", ParseErrorCode::kInvalidValue, "field2"},
        UInt64FromJsonFailureTestParam{R"({"field1":"1e50","field2":1})", ParseErrorCode::kInvalidValue, "field1"}
    )
);

TEST_P(UInt64FromJsonSuccessTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::UInt64Message message, expected_message, sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);
    expected_message = PrepareTestData(param.expected_message);

    message.set_field1(100001);
    message.set_field2(200002);

    UASSERT_NO_THROW((message = JsonToMessage<proto_json::messages::UInt64Message>(input, param.options)));

#if GOOGLE_PROTOBUF_VERSION >= 6030000
    UASSERT_NO_THROW(InitSampleMessage(param.input, sample_message, param.options));
    CheckMessageEqual(message, sample_message);
#else
    if (!param.skip_native_parsing_for_older_versions) {
        UASSERT_NO_THROW(InitSampleMessage(param.input, sample_message, param.options));
        CheckMessageEqual(message, sample_message);
    }
#endif

    CheckMessageEqual(message, expected_message);
}

TEST_P(UInt64FromJsonFailureTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::UInt64Message sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);

    EXPECT_PARSE_ERROR(
        (void)JsonToMessage<proto_json::messages::UInt64Message>(input, param.options),
        param.expected_errc,
        param.expected_path
    );
    UEXPECT_THROW(InitSampleMessage(param.input, sample_message, param.options), SampleError);
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
