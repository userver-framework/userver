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

constexpr std::int32_t kMax = std::numeric_limits<std::int32_t>::max();  // 2147483647
constexpr std::int32_t kMin = std::numeric_limits<std::int32_t>::min();  // -2147483648

struct Int32FromJsonSuccessTestParam {
    std::string input = {};
    Int32MessageData expected_message = {};
    ParseOptions options = {};

    // Older protobuf libraries does not support exponential notation for the quoted integers.
    // This was fixed in the "v30.0-rc1" / "v6.30.0-rc1" (GOOGLE_PROTOBUF_VERSION = 6030000).
    bool skip_native_parsing_for_older_versions = false;
};

struct Int32FromJsonFailureTestParam {
    std::string input = {};
    ParseErrorCode expected_errc = {};
    std::string expected_path = {};
    ParseOptions options = {};
};

void PrintTo(const Int32FromJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

void PrintTo(const Int32FromJsonFailureTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

class Int32FromJsonSuccessTest : public ::testing::TestWithParam<Int32FromJsonSuccessTestParam> {};
class Int32FromJsonFailureTest : public ::testing::TestWithParam<Int32FromJsonFailureTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    Int32FromJsonSuccessTest,
    ::testing::Values(
        Int32FromJsonSuccessTestParam{R"({})", Int32MessageData{0, 0, 0}},
        Int32FromJsonSuccessTestParam{R"({"field1":0,"field2":null,"field3":"0"})", Int32MessageData{0, 0, 0}},
        Int32FromJsonSuccessTestParam{R"({"field1":1,"field2":-2,"field3":3})", Int32MessageData{1, -2, 3}},
        Int32FromJsonSuccessTestParam{R"({"field1":"1","field2":"-2","field3":"3"})", Int32MessageData{1, -2, 3}},
        Int32FromJsonSuccessTestParam{
            R"({"field1":2147483647,"field2":"2147483647","field3":-2147483648})",
            Int32MessageData{kMax, kMax, kMin}
        },
        Int32FromJsonSuccessTestParam{
            R"({"field1":-2147483648,"field2":"-2147483648","field3":2147483647})",
            Int32MessageData{kMin, kMin, kMax}
        },
        Int32FromJsonSuccessTestParam{
            R"({"field1":10e2,"field2":-5e3,"field3":3e0})",
            Int32MessageData{1000, -5000, 3}
        },
        Int32FromJsonSuccessTestParam{
            R"({"field1":"10e2","field2":"-5e3","field3":"3e0"})",
            Int32MessageData{1000, -5000, 3},
            {},
            true
        },
        Int32FromJsonSuccessTestParam{
            R"({"field1":8500E-2,"field2":-1.8E+2,"field3":6.0E0})",
            Int32MessageData{85, -180, 6}
        },
        Int32FromJsonSuccessTestParam{
            R"({"field1":"8500E-2","field2":"-1.8E+2","field3":"6.0E0"})",
            Int32MessageData{85, -180, 6},
            {},
            true
        }
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    Int32FromJsonFailureTest,
    ::testing::Values(
        Int32FromJsonFailureTestParam{R"({"field1":[],"field2":1,"field3":1})", ParseErrorCode::kInvalidType, "field1"},
        Int32FromJsonFailureTestParam{R"({"field1":1,"field2":{},"field3":1})", ParseErrorCode::kInvalidType, "field2"},
        Int32FromJsonFailureTestParam{
            R"({"field1":1,"field2":1,"field3":true})",
            ParseErrorCode::kInvalidType,
            "field3"
        },
        Int32FromJsonFailureTestParam{
            R"({"field1":2147483648,"field2":1,"field3":1})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        Int32FromJsonFailureTestParam{
            R"({"field1":1,"field2":-2147483649,"field3":1})",
            ParseErrorCode::kInvalidValue,
            "field2"
        },
        Int32FromJsonFailureTestParam{
            R"({"field1":1,"field2":1,"field3":"2147483648"})",
            ParseErrorCode::kInvalidValue,
            "field3"
        },
        Int32FromJsonFailureTestParam{
            R"({"field1":"-2147483649","field2":1,"field3":1})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        Int32FromJsonFailureTestParam{
            R"({"field1":1,"field2":2e12,"field3":1})",
            ParseErrorCode::kInvalidValue,
            "field2"
        },
        Int32FromJsonFailureTestParam{
            R"({"field1":1,"field2":1,"field3":-2e12})",
            ParseErrorCode::kInvalidValue,
            "field3"
        },
        Int32FromJsonFailureTestParam{
            R"({"field1":" 123","field2":1,"field3":1})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        Int32FromJsonFailureTestParam{
            R"({"field1":1,"field2":"-123 ","field3":1})",
            ParseErrorCode::kInvalidValue,
            "field2"
        },
        Int32FromJsonFailureTestParam{
            R"({"field1":1,"field2":1,"field3":"1a3"})",
            ParseErrorCode::kInvalidValue,
            "field3"
        },
        Int32FromJsonFailureTestParam{
            R"({"field1":"1.1","field2":1,"field3":1})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        Int32FromJsonFailureTestParam{
            R"({"field1":1,"field2":"2e12","field3":1})",
            ParseErrorCode::kInvalidValue,
            "field2"
        },
        Int32FromJsonFailureTestParam{
            R"({"field1":1,"field2":1,"field3":"-2e12"})",
            ParseErrorCode::kInvalidValue,
            "field3"
        },
        Int32FromJsonFailureTestParam{
            R"({"field1":"1e50","field2":1,"field3":1})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        Int32FromJsonFailureTestParam{
            R"({"field1":1,"field2":"-1e50","field3":1})",
            ParseErrorCode::kInvalidValue,
            "field2"
        }
    )
);

TEST_P(Int32FromJsonSuccessTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::Int32Message message, expected_message, sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);
    expected_message = PrepareTestData(param.expected_message);

    message.set_field1(100001);
    message.set_field2(200002);
    message.set_field3(300003);

    UASSERT_NO_THROW((message = JsonToMessage<proto_json::messages::Int32Message>(input, param.options)));

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

TEST_P(Int32FromJsonFailureTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::Int32Message sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);

    EXPECT_PARSE_ERROR(
        (void)JsonToMessage<proto_json::messages::Int32Message>(input, param.options),
        param.expected_errc,
        param.expected_path
    );
    UEXPECT_THROW(InitSampleMessage(param.input, sample_message, param.options), SampleError);
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
