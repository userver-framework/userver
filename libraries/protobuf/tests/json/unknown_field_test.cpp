#include <gtest/gtest.h>

#include <ostream>
#include <string>

#include <fmt/format.h>

#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

struct UnknownJsonFieldAcceptedTestParam {
    std::string input = {};
    UnknownFieldMessageData expected_message = {};
};

struct UnknownJsonFieldRejectedTestParam {
    std::string input = {};
    std::string expected_path = {};
};

void PrintTo(const UnknownJsonFieldAcceptedTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

void PrintTo(const UnknownJsonFieldRejectedTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

class UnknownJsonFieldAcceptedTest : public ::testing::TestWithParam<UnknownJsonFieldAcceptedTestParam> {};
class UnknownJsonFieldRejectedTest : public ::testing::TestWithParam<UnknownJsonFieldRejectedTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    UnknownJsonFieldAcceptedTest,
    ::testing::Values(
        UnknownJsonFieldAcceptedTestParam{
            R"({
              "field1":{"field1":"aaa"},
              "field2":[{"field1":"item1"}],
              "field3":{"1":{"field1":"value1"}}
            })",
            UnknownFieldMessageData{{"aaa"}, {{"item1"}}, {{1, {"value1"}}}}
        },
        UnknownJsonFieldAcceptedTestParam{
            R"({
              "unknown_field": true,
              "field1":{"field1":"aaa"},
              "field2":[{"field1":"item1"}],
              "field3":{"1":{"field1":"value1"}}
            })",
            UnknownFieldMessageData{{"aaa"}, {{"item1"}}, {{1, {"value1"}}}}
        },
        UnknownJsonFieldAcceptedTestParam{
            R"({
              "field1":{"field1":"aaa","unknown_field":true},
              "field2":[{"field1":"item1"}],
              "field3":{"1":{"field1":"value1"}}
            })",
            UnknownFieldMessageData{{"aaa"}, {{"item1"}}, {{1, {"value1"}}}}
        },
        UnknownJsonFieldAcceptedTestParam{
            R"({
              "field1":{"field1":"aaa"},
              "field2":[{"field1":"item1","unknown_field":true}],
              "field3":{"1":{"field1":"value1"}}
            })",
            UnknownFieldMessageData{{"aaa"}, {{"item1"}}, {{1, {"value1"}}}}
        },
        UnknownJsonFieldAcceptedTestParam{
            R"({
              "field1":{"field1":"aaa"},
              "field2":[{"field1":"item1"}],
              "field3":{"1":{"field1":"value1","unknown_field":true}}
            })",
            UnknownFieldMessageData{{"aaa"}, {{"item1"}}, {{1, {"value1"}}}}
        }
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    UnknownJsonFieldRejectedTest,
    ::testing::Values(
        UnknownJsonFieldRejectedTestParam{
            R"({
              "unknown.field": true,
              "field1":{"field1":"aaa"},
              "field2":[{"field1":"item1"}],
              "field3":{"1":{"field1":"value1"}}
            })",
            "'unknown.field'"
        },
        UnknownJsonFieldRejectedTestParam{
            R"({
              "field1":{"field1":"aaa","unknown.field":true},
              "field2":[{"field1":"item1"}],
              "field3":{"1":{"field1":"value1"}}
            })",
            "field1.'unknown.field'"
        },
        UnknownJsonFieldRejectedTestParam{
            R"({
              "field1":{"field1":"aaa"},
              "field2":[{"field1":"item1","unknown.field":true}],
              "field3":{"1":{"field1":"value1"}}
            })",
            "field2[0].'unknown.field'",
        },
        UnknownJsonFieldRejectedTestParam{
            R"({
              "field1":{"field1":"aaa"},
              "field2":[{"field1":"item1"}],
              "field3":{"1":{"field1":"value1","unknown.field":true}}
            })",
            "field3.1.'unknown.field'"
        }
    )
);

TEST_P(UnknownJsonFieldAcceptedTest, Test) {
    using Message = proto_json::messages::UnknownFieldMessage;
    const auto& param = GetParam();

    Message message;
    Message expected_message;
    Message sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);
    expected_message = PrepareTestData(param.expected_message);

    UASSERT_NO_THROW((message = JsonToMessage<Message>(input, {.ignore_unknown_fields = true})));
    UASSERT_NO_THROW(InitSampleMessage(param.input, sample_message, {.ignore_unknown_fields = true}));

    CheckMessageEqual(message, sample_message);
    CheckMessageEqual(message, expected_message);
}

TEST_P(UnknownJsonFieldRejectedTest, Test) {
    using Message = proto_json::messages::UnknownFieldMessage;
    const auto& param = GetParam();

    Message sample;
    formats::json::Value input = PrepareJsonTestData(param.input);

    EXPECT_PARSE_ERROR(
        (void)JsonToMessage<proto_json::messages::UnknownFieldMessage>(input, {.ignore_unknown_fields = false}),
        ParseErrorCode::kUnknownField,
        param.expected_path
    );
    UEXPECT_THROW(InitSampleMessage(param.input, sample, {.ignore_unknown_fields = false}), SampleError);
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
