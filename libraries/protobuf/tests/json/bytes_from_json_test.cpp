#include <gtest/gtest.h>

#include <ostream>
#include <string>

#include <fmt/format.h>

#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utils/encoding/hex.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

struct BytesFromJsonSuccessTestParam {
    std::string input = {};
    BytesMessageData expected_message = {};
    ReadOptions options = {};
};

struct BytesFromJsonFailureTestParam {
    std::string input = {};
    ReadErrorCode expected_errc = {};
    std::string expected_path = {};
    ReadOptions options = {};
};

void PrintTo(const BytesFromJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

void PrintTo(const BytesFromJsonFailureTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

class BytesFromJsonSuccessTest : public ::testing::TestWithParam<BytesFromJsonSuccessTestParam> {};
class BytesFromJsonFailureTest : public ::testing::TestWithParam<BytesFromJsonFailureTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    BytesFromJsonSuccessTest,
    ::testing::Values(
        BytesFromJsonSuccessTestParam{R"({})", BytesMessageData{""}},
        BytesFromJsonSuccessTestParam{R"({"field1":null})", BytesMessageData{""}},
        BytesFromJsonSuccessTestParam{R"({"field1":""})", BytesMessageData{""}},
        BytesFromJsonSuccessTestParam{R"({"field1":"+/s="})", BytesMessageData{std::string("\xfb\xfb", 2)}},
        BytesFromJsonSuccessTestParam{R"({"field1":"-_s="})", BytesMessageData{std::string("\xfb\xfb", 2)}},
        BytesFromJsonSuccessTestParam{R"({"field1":"+/s"})", BytesMessageData{std::string("\xfb\xfb", 2)}},
        BytesFromJsonSuccessTestParam{R"({"field1":"-_s"})", BytesMessageData{std::string("\xfb\xfb", 2)}}
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    BytesFromJsonFailureTest,
    ::testing::Values(
        BytesFromJsonFailureTestParam{R"({"field1":[]})", ReadErrorCode::kInvalidType, "field1"},
        BytesFromJsonFailureTestParam{R"({"field1":{}})", ReadErrorCode::kInvalidType, "field1"},
        BytesFromJsonFailureTestParam{R"({"field1":1})", ReadErrorCode::kInvalidType, "field1"},
        BytesFromJsonFailureTestParam{R"({"field1":true})", ReadErrorCode::kInvalidType, "field1"},
        BytesFromJsonFailureTestParam{R"({"field1":"TW=E"})", ReadErrorCode::kInvalidValue, "field1"},
        BytesFromJsonFailureTestParam{R"({"field1":"TQ==="})", ReadErrorCode::kInvalidValue, "field1"},
        BytesFromJsonFailureTestParam{R"({"field1":"TWE#"})", ReadErrorCode::kInvalidValue, "field1"}
    )
);

TEST_P(BytesFromJsonSuccessTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::BytesMessage message, expected_message, sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);
    expected_message = PrepareTestData(param.expected_message);

    message.set_field1("dump");

    UASSERT_NO_THROW((message = JsonToMessage<proto_json::messages::BytesMessage>(input, param.options)));
    UASSERT_NO_THROW(InitSampleMessage(param.input, param.options, sample_message));

    CheckMessageEqual(message, sample_message);
    CheckMessageEqual(message, expected_message);
}

TEST_P(BytesFromJsonFailureTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::BytesMessage sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);

    EXPECT_READ_ERROR(
        (void)JsonToMessage<proto_json::messages::BytesMessage>(input, param.options),
        param.expected_errc,
        param.expected_path
    );
    UEXPECT_THROW(InitSampleMessage(param.input, param.options, sample_message), SampleError);
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
