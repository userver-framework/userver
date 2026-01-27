#include <gtest/gtest.h>

#include <ostream>
#include <string>

#include <fmt/format.h>

#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

struct UnkownJsonFieldAcceptedTestParam {
    std::string input = {};
    UnknownFieldMessageData expected_message = {};
};

struct UnkownJsonFieldRejectedTestParam {
    std::string input = {};
    std::string expected_path = {};
};

void PrintTo(const UnkownJsonFieldAcceptedTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

void PrintTo(const UnkownJsonFieldRejectedTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

class UnkownJsonFieldAcceptedTest : public ::testing::TestWithParam<UnkownJsonFieldAcceptedTestParam> {};
class UnkownJsonFieldRejectedTest : public ::testing::TestWithParam<UnkownJsonFieldRejectedTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    UnkownJsonFieldAcceptedTest,
    ::testing::Values(
        UnkownJsonFieldAcceptedTestParam{
            R"({
              "field1":{"field1":"aaa"},
              "field2":[{"field1":"item1"}],
              "field3":{"1":{"field1":"value1"}}
            })",
            UnknownFieldMessageData{{"aaa"}, {{"item1"}}, {{1, {"value1"}}}}
        },
        UnkownJsonFieldAcceptedTestParam{
            R"({
              "unknown_field": true,
              "field1":{"field1":"aaa"},
              "field2":[{"field1":"item1"}],
              "field3":{"1":{"field1":"value1"}}
            })",
            UnknownFieldMessageData{{"aaa"}, {{"item1"}}, {{1, {"value1"}}}}
        },
        UnkownJsonFieldAcceptedTestParam{
            R"({
              "field1":{"field1":"aaa","unknown_field":true},
              "field2":[{"field1":"item1"}],
              "field3":{"1":{"field1":"value1"}}
            })",
            UnknownFieldMessageData{{"aaa"}, {{"item1"}}, {{1, {"value1"}}}}
        },
        UnkownJsonFieldAcceptedTestParam{
            R"({
              "field1":{"field1":"aaa"},
              "field2":[{"field1":"item1","unknown_field":true}],
              "field3":{"1":{"field1":"value1"}}
            })",
            UnknownFieldMessageData{{"aaa"}, {{"item1"}}, {{1, {"value1"}}}}
        },
        UnkownJsonFieldAcceptedTestParam{
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
    UnkownJsonFieldRejectedTest,
    ::testing::Values(
        UnkownJsonFieldRejectedTestParam{
            R"({
              "unknown_field": true,
              "field1":{"field1":"aaa"},
              "field2":[{"field1":"item1"}],
              "field3":{"1":{"field1":"value1"}}
            })",
            "unknown_field"
        },
        UnkownJsonFieldRejectedTestParam{
            R"({
              "field1":{"field1":"aaa","unknown_field":true},
              "field2":[{"field1":"item1"}],
              "field3":{"1":{"field1":"value1"}}
            })",
            "field1.unknown_field"
        },
        UnkownJsonFieldRejectedTestParam{
            R"({
              "field1":{"field1":"aaa"},
              "field2":[{"field1":"item1","unknown_field":true}],
              "field3":{"1":{"field1":"value1"}}
            })",
            "field2[0].unknown_field",
        },
        UnkownJsonFieldRejectedTestParam{
            R"({
              "field1":{"field1":"aaa"},
              "field2":[{"field1":"item1"}],
              "field3":{"1":{"field1":"value1","unknown_field":true}}
            })",
            "field3.1.unknown_field"
        }
    )
);

TEST_P(UnkownJsonFieldAcceptedTest, Test) {
    using Message = proto_json::messages::UnknownFieldMessage;
    const auto& param = GetParam();

    Message message, expected_message, sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);
    expected_message = PrepareTestData(param.expected_message);

    UASSERT_NO_THROW((message = JsonToMessage<Message>(input, {.ignore_unknown_fields = true})));
    UASSERT_NO_THROW(InitSampleMessage(param.input, sample_message, {.ignore_unknown_fields = true}));

    CheckMessageEqual(message, sample_message);
    CheckMessageEqual(message, expected_message);
}

TEST_P(UnkownJsonFieldRejectedTest, Test) {
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
