#include <gtest/gtest.h>

#include <ostream>
#include <string>
#include <unordered_set>

#include <fmt/format.h>

#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

struct OneofFromJsonSuccessTestParam {
    std::string input = {};
    OneofMessageData expected_message = {};
    ParseOptions options = {};
};

struct OneofFromJsonFailureTestParam {
    std::string input = {};
    ParseErrorCode expected_errc = {};
    std::unordered_set<std::string> expected_path = {};
    ParseOptions options = {};
};

void PrintTo(const OneofFromJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

void PrintTo(const OneofFromJsonFailureTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

class OneofFromJsonSuccessTest : public ::testing::TestWithParam<OneofFromJsonSuccessTestParam> {};
class OneofFromJsonFailureTest : public ::testing::TestWithParam<OneofFromJsonFailureTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    OneofFromJsonSuccessTest,
    ::testing::Values(
        OneofFromJsonSuccessTestParam{R"({})", OneofMessageData{}},
        OneofFromJsonSuccessTestParam{R"({"field1":10})", OneofMessageData{.field1 = 10}},
        OneofFromJsonSuccessTestParam{R"({"field2":"hello"})", OneofMessageData{.field2 = "hello"}}
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    OneofFromJsonFailureTest,
    ::testing::Values(OneofFromJsonFailureTestParam{
        R"({"field1":10,"field2":"hello"})",
        ParseErrorCode::kMultipleOneofFields,
        {"field1", "field2"}
    })
);

TEST_P(OneofFromJsonSuccessTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::OneofMessage message, expected_message, sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);
    expected_message = PrepareTestData(param.expected_message);

    UASSERT_NO_THROW((message = JsonToMessage<proto_json::messages::OneofMessage>(input, param.options)));
    UASSERT_NO_THROW(InitSampleMessage(param.input, sample_message, param.options));

    CheckMessageEqual(message, sample_message);
    CheckMessageEqual(message, expected_message);
}

TEST_P(OneofFromJsonFailureTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::OneofMessage sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);

    try {
        (void)JsonToMessage<proto_json::messages::OneofMessage>(input, param.options);
        ADD_FAILURE() << "exception should be thrown";
    } catch (const ParseError& error) {
        EXPECT_EQ(error.GetErrorInfo().GetCode(), param.expected_errc);
        EXPECT_TRUE(param.expected_path.contains(error.GetErrorInfo().GetPath()));
    } catch (...) {
        ADD_FAILURE() << "unexpected exception type";
    }

    UEXPECT_THROW(InitSampleMessage(param.input, sample_message, param.options), SampleError);
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
