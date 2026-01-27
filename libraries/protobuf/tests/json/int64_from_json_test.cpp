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

constexpr std::int64_t kMax = std::numeric_limits<std::int64_t>::max();  //  9223372036854775807
constexpr std::int64_t kMin = std::numeric_limits<std::int64_t>::min();  // -9223372036854775808

struct Int64FromJsonSuccessTestParam {
    std::string input = {};
    Int64MessageData expected_message = {};
    ParseOptions options = {};

    // Older protobuf libraries does not support exponential notation for the quoted integers.
    // This was fixed in the "v30.0-rc1" / "v6.30.0-rc1" (GOOGLE_PROTOBUF_VERSION = 6030000).
    bool skip_native_parsing_for_older_versions = false;
};

struct Int64FromJsonFailureTestParam {
    std::string input = {};
    ParseErrorCode expected_errc = {};
    std::string expected_path = {};
    ParseOptions options = {};
};

void PrintTo(const Int64FromJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

void PrintTo(const Int64FromJsonFailureTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

class Int64FromJsonSuccessTest : public ::testing::TestWithParam<Int64FromJsonSuccessTestParam> {};
class Int64FromJsonFailureTest : public ::testing::TestWithParam<Int64FromJsonFailureTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    Int64FromJsonSuccessTest,
    ::testing::Values(
        Int64FromJsonSuccessTestParam{R"({})", Int64MessageData{0, 0, 0}},
        Int64FromJsonSuccessTestParam{R"({"field1":0,"field2":null,"field3":"0"})", Int64MessageData{0, 0, 0}},
        Int64FromJsonSuccessTestParam{R"({"field1":1,"field2":-2,"field3":3})", Int64MessageData{1, -2, 3}},
        Int64FromJsonSuccessTestParam{R"({"field1":"1","field2":"-2","field3":"3"})", Int64MessageData{1, -2, 3}},
        Int64FromJsonSuccessTestParam{
            R"({"field1":9223372036854775807,"field2":"9223372036854775807","field3":-9223372036854775808})",
            Int64MessageData{kMax, kMax, kMin}
        },
        Int64FromJsonSuccessTestParam{
            R"({"field1":-9223372036854775808,"field2":"-9223372036854775808","field3":9223372036854775807})",
            Int64MessageData{kMin, kMin, kMax}
        },
        Int64FromJsonSuccessTestParam{
            R"({"field1":10e2,"field2":-5e3,"field3":3e0})",
            Int64MessageData{1000, -5000, 3}
        },
        Int64FromJsonSuccessTestParam{
            R"({"field1":"10e2","field2":"-5e3","field3":"3e0"})",
            Int64MessageData{1000, -5000, 3},
            {},
            true
        },
        Int64FromJsonSuccessTestParam{
            R"({"field1":8500E-2,"field2":-1.8E+2,"field3":6.0E0})",
            Int64MessageData{85, -180, 6}
        },
        Int64FromJsonSuccessTestParam{
            R"({"field1":"8500E-2","field2":"-1.8E+2","field3":"6.0E0"})",
            Int64MessageData{85, -180, 6},
            {},
            true
        }
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    Int64FromJsonFailureTest,
    ::testing::Values(
        Int64FromJsonFailureTestParam{R"({"field1":[],"field2":1,"field3":1})", ParseErrorCode::kInvalidType, "field1"},
        Int64FromJsonFailureTestParam{R"({"field1":1,"field2":{},"field3":1})", ParseErrorCode::kInvalidType, "field2"},
        Int64FromJsonFailureTestParam{
            R"({"field1":1,"field2":1,"field3":true})",
            ParseErrorCode::kInvalidType,
            "field3"
        },
        Int64FromJsonFailureTestParam{
            R"({"field1":9223372036854775808,"field2":1,"field3":1})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        Int64FromJsonFailureTestParam{
            R"({"field1":1,"field2":-9223372036854775809,"field3":1})",
            ParseErrorCode::kInvalidValue,
            "field2"
        },
        Int64FromJsonFailureTestParam{
            R"({"field1":1,"field2":1,"field3":"9223372036854775808"})",
            ParseErrorCode::kInvalidValue,
            "field3"
        },
        Int64FromJsonFailureTestParam{
            R"({"field1":"-9223372036854775809","field2":1,"field3":1})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        Int64FromJsonFailureTestParam{
            R"({"field1":1,"field2":1e50,"field3":1})",
            ParseErrorCode::kInvalidValue,
            "field2"
        },
        Int64FromJsonFailureTestParam{
            R"({"field1":1,"field2":1,"field3":-1e50})",
            ParseErrorCode::kInvalidValue,
            "field3"
        },
        Int64FromJsonFailureTestParam{
            R"({"field1":" 123","field2":1,"field3":1})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        Int64FromJsonFailureTestParam{
            R"({"field1":1,"field2":"-123 ","field3":1})",
            ParseErrorCode::kInvalidValue,
            "field2"
        },
        Int64FromJsonFailureTestParam{
            R"({"field1":1,"field2":1,"field3":"1a3"})",
            ParseErrorCode::kInvalidValue,
            "field3"
        },
        Int64FromJsonFailureTestParam{
            R"({"field1":"1.1","field2":1,"field3":1})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        Int64FromJsonFailureTestParam{
            R"({"field1":1,"field2":"1e50","field3":1})",
            ParseErrorCode::kInvalidValue,
            "field2"
        },
        Int64FromJsonFailureTestParam{
            R"({"field1":1,"field2":1,"field3":"-1e50"})",
            ParseErrorCode::kInvalidValue,
            "field3"
        }
    )
);

TEST_P(Int64FromJsonSuccessTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::Int64Message message, expected_message, sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);
    expected_message = PrepareTestData(param.expected_message);

    message.set_field1(100001);
    message.set_field2(200002);
    message.set_field3(300003);

    UASSERT_NO_THROW((message = JsonToMessage<proto_json::messages::Int64Message>(input, param.options)));

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

TEST_P(Int64FromJsonFailureTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::Int64Message sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);

    EXPECT_PARSE_ERROR(
        (void)JsonToMessage<proto_json::messages::Int64Message>(input, param.options),
        param.expected_errc,
        param.expected_path
    );
    UEXPECT_THROW(InitSampleMessage(param.input, sample_message, param.options), SampleError);
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
