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

constexpr std::uint32_t kMax = std::numeric_limits<std::uint32_t>::max();  // 4294967295

struct UInt32FromJsonSuccessTestParam {
    std::string input = {};
    UInt32MessageData expected_message = {};
    ParseOptions options = {};

    // Older protobuf libraries does not support exponential notation for the quoted integers.
    // This was fixed in the "v30.0-rc1" / "v6.30.0-rc1" (GOOGLE_PROTOBUF_VERSION = 6030000).
    bool skip_native_parsing_for_older_versions = false;
};

struct UInt32FromJsonFailureTestParam {
    std::string input = {};
    ParseErrorCode expected_errc = {};
    std::string expected_path = {};
    ParseOptions options = {};
};

void PrintTo(const UInt32FromJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

void PrintTo(const UInt32FromJsonFailureTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

class UInt32FromJsonSuccessTest : public ::testing::TestWithParam<UInt32FromJsonSuccessTestParam> {};
class UInt32FromJsonFailureTest : public ::testing::TestWithParam<UInt32FromJsonFailureTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    UInt32FromJsonSuccessTest,
    ::testing::Values(
        UInt32FromJsonSuccessTestParam{R"({})", UInt32MessageData{0, 0}},
        UInt32FromJsonSuccessTestParam{R"({"field1":null,"field2":null})", UInt32MessageData{0, 0}},
        UInt32FromJsonSuccessTestParam{R"({"field1":0,"field2":1})", UInt32MessageData{0, 1}},
        UInt32FromJsonSuccessTestParam{R"({"field1":"0","field2":"1"})", UInt32MessageData{0, 1}},
        UInt32FromJsonSuccessTestParam{R"({"field1":4294967295,"field2":"4294967295"})", UInt32MessageData{kMax, kMax}},
        UInt32FromJsonSuccessTestParam{R"({"field1":10e2,"field2":3e0})", UInt32MessageData{1000, 3}},
        UInt32FromJsonSuccessTestParam{R"({"field1":"10e2","field2":"3e0"})", UInt32MessageData{1000, 3}, {}, true},
        UInt32FromJsonSuccessTestParam{R"({"field1":8500E-2,"field2":1.8E+2})", UInt32MessageData{85, 180}},
        UInt32FromJsonSuccessTestParam{
            R"({"field1":"8500E-2","field2":"1.8E+2"})",
            UInt32MessageData{85, 180},
            {},
            true
        }
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    UInt32FromJsonFailureTest,
    ::testing::Values(
        UInt32FromJsonFailureTestParam{R"({"field1":[],"field2":1})", ParseErrorCode::kInvalidType, "field1"},
        UInt32FromJsonFailureTestParam{R"({"field1":1,"field2":{}})", ParseErrorCode::kInvalidType, "field2"},
        UInt32FromJsonFailureTestParam{R"({"field1":1,"field2":true})", ParseErrorCode::kInvalidType, "field2"},
        UInt32FromJsonFailureTestParam{R"({"field1":4294967296,"field2":1})", ParseErrorCode::kInvalidValue, "field1"},
        UInt32FromJsonFailureTestParam{R"({"field1":1,"field2":-1})", ParseErrorCode::kInvalidValue, "field2"},
        UInt32FromJsonFailureTestParam{
            R"({"field1":"4294967296","field2":1})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        UInt32FromJsonFailureTestParam{R"({"field1":1,"field2":"-1"})", ParseErrorCode::kInvalidValue, "field2"},
        UInt32FromJsonFailureTestParam{R"({"field1":2e12,"field2":1})", ParseErrorCode::kInvalidValue, "field1"},
        UInt32FromJsonFailureTestParam{R"({"field1":1,"field2":-2e12})", ParseErrorCode::kInvalidValue, "field2"},
        UInt32FromJsonFailureTestParam{R"({"field1":" 123","field2":1})", ParseErrorCode::kInvalidValue, "field1"},
        UInt32FromJsonFailureTestParam{R"({"field1":1,"field2":"123 "})", ParseErrorCode::kInvalidValue, "field2"},
        UInt32FromJsonFailureTestParam{R"({"field1":"1a3","field2":1})", ParseErrorCode::kInvalidValue, "field1"},
        UInt32FromJsonFailureTestParam{R"({"field1":"1.1","field2":1})", ParseErrorCode::kInvalidValue, "field1"},
        UInt32FromJsonFailureTestParam{R"({"field1":1,"field2":"2e12"})", ParseErrorCode::kInvalidValue, "field2"},
        UInt32FromJsonFailureTestParam{R"({"field1":"-2e12","field2":1})", ParseErrorCode::kInvalidValue, "field1"},
        UInt32FromJsonFailureTestParam{R"({"field1":1,"field2":"-1e50"})", ParseErrorCode::kInvalidValue, "field2"},
        UInt32FromJsonFailureTestParam{R"({"field1":"1e50","field2":1})", ParseErrorCode::kInvalidValue, "field1"}
    )
);

TEST_P(UInt32FromJsonSuccessTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::UInt32Message message, expected_message, sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);
    expected_message = PrepareTestData(param.expected_message);

    message.set_field1(100001);
    message.set_field2(200002);

    UASSERT_NO_THROW((message = JsonToMessage<proto_json::messages::UInt32Message>(input, param.options)));

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

TEST_P(UInt32FromJsonFailureTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::UInt32Message sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);

    EXPECT_PARSE_ERROR(
        (void)JsonToMessage<proto_json::messages::UInt32Message>(input, param.options),
        param.expected_errc,
        param.expected_path
    );
    UEXPECT_THROW(InitSampleMessage(param.input, sample_message, param.options), SampleError);
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
