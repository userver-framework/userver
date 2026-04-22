#include <gtest/gtest.h>

#include <ostream>
#include <string>

#include <fmt/format.h>

#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

struct NestedFromJsonSuccessTestParam {
    std::string input = {};
    NestedMessageData expected_message = {};
    ReadOptions options = {};
};

struct NestedFromJsonFailureTestParam {
    std::string input = {};
    ReadErrorCode expected_errc = {};
    std::string expected_path = {};
    ReadOptions options = {};
};

void PrintTo(const NestedFromJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

void PrintTo(const NestedFromJsonFailureTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

class NestedFromJsonSuccessTest : public ::testing::TestWithParam<NestedFromJsonSuccessTestParam> {};
class NestedFromJsonFailureTest : public ::testing::TestWithParam<NestedFromJsonFailureTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    NestedFromJsonSuccessTest,
    ::testing::Values(
        NestedFromJsonSuccessTestParam{R"({})", NestedMessageData{std::nullopt}},
        NestedFromJsonSuccessTestParam{R"({"parent":null})", NestedMessageData{std::nullopt}},
        NestedFromJsonSuccessTestParam{R"({"parent":{}})", NestedMessageData{0}},
        NestedFromJsonSuccessTestParam{R"({"parent":{"field1":null}})", NestedMessageData{0}},
        NestedFromJsonSuccessTestParam{R"({"parent":{"field1":123}})", NestedMessageData{123}}
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    NestedFromJsonFailureTest,
    ::testing::Values(
        NestedFromJsonFailureTestParam{R"(null)", ReadErrorCode::kInvalidType, "/"},
        NestedFromJsonFailureTestParam{R"([])", ReadErrorCode::kInvalidType, "/"},
        NestedFromJsonFailureTestParam{R"(true)", ReadErrorCode::kInvalidType, "/"},
        NestedFromJsonFailureTestParam{R"(10)", ReadErrorCode::kInvalidType, "/"},
        NestedFromJsonFailureTestParam{R"("test")", ReadErrorCode::kInvalidType, "/"},
        NestedFromJsonFailureTestParam{R"({"parent":[]})", ReadErrorCode::kInvalidType, "parent"},
        NestedFromJsonFailureTestParam{R"({"parent":true})", ReadErrorCode::kInvalidType, "parent"},
        NestedFromJsonFailureTestParam{R"({"parent":10})", ReadErrorCode::kInvalidType, "parent"},
        NestedFromJsonFailureTestParam{R"({"parent":"test"})", ReadErrorCode::kInvalidType, "parent"},
        NestedFromJsonFailureTestParam{
            R"({"parent":{"field1":2147483648}})",
            ReadErrorCode::kInvalidValue,
            "parent.field1"
        },
        NestedFromJsonFailureTestParam{
            R"({"parent":{"field1":-2147483649}})",
            ReadErrorCode::kInvalidValue,
            "parent.field1"
        }
    )
);

TEST_P(NestedFromJsonSuccessTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::NestedMessage message, expected_message, sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);
    expected_message = PrepareTestData(param.expected_message);

    message.mutable_parent()->set_field1(100001);

    UASSERT_NO_THROW((message = JsonToMessage<proto_json::messages::NestedMessage>(input, param.options)));
    UASSERT_NO_THROW(InitSampleMessage(param.input, param.options, sample_message));

    CheckMessageEqual(message, sample_message);
    CheckMessageEqual(message, expected_message);
}

TEST_P(NestedFromJsonFailureTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::NestedMessage sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);

    EXPECT_READ_ERROR(
        (void)JsonToMessage<proto_json::messages::NestedMessage>(input, param.options),
        param.expected_errc,
        param.expected_path
    );
    UEXPECT_THROW(InitSampleMessage(param.input, param.options, sample_message), SampleError);
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
