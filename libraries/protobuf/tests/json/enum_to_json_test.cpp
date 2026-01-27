#include <gtest/gtest.h>

#include <ostream>
#include <string>

#include <fmt/format.h>

#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

namespace msgs = proto_json::messages;

struct EnumToJsonSuccessTestParam {
    EnumMessageData input = {};
    std::string expected_json = {};
    PrintOptions options = {};
};

void PrintTo(const EnumToJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = {{.field1={}}} }}", static_cast<int>(param.input.field1));
}

class EnumToJsonSuccessTest : public ::testing::TestWithParam<EnumToJsonSuccessTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    EnumToJsonSuccessTest,
    ::testing::Values(
        EnumToJsonSuccessTestParam{EnumMessageData{msgs::EnumMessage::TEST_UNSPECIFIED}, R"({})"},
        EnumToJsonSuccessTestParam{
            EnumMessageData{msgs::EnumMessage::TEST_UNSPECIFIED},
            R"({"field1":"TEST_UNSPECIFIED"})",
            {.always_print_fields_with_no_presence = true}
        },
        EnumToJsonSuccessTestParam{EnumMessageData{msgs::EnumMessage::TEST_VALUE1}, R"({"field1":"TEST_VALUE1"})"},
        EnumToJsonSuccessTestParam{EnumMessageData{msgs::EnumMessage::TEST_VALUE2}, R"({"field1":"TEST_VALUE2"})"},
        EnumToJsonSuccessTestParam{
            EnumMessageData{msgs::EnumMessage::TEST_VALUE2_ALIAS},
            R"({"field1":"TEST_VALUE2"})"
        },
        EnumToJsonSuccessTestParam{EnumMessageData{msgs::EnumMessage::TEST_VALUE3}, R"({"field1":"TEST_VALUE3"})"},
        EnumToJsonSuccessTestParam{
            EnumMessageData{msgs::EnumMessage::TEST_UNSPECIFIED},
            R"({"field1":0})",
            {.always_print_fields_with_no_presence = true, .always_print_enums_as_ints = true}
        },
        EnumToJsonSuccessTestParam{
            EnumMessageData{msgs::EnumMessage::TEST_VALUE1},
            R"({"field1":1})",
            {.always_print_enums_as_ints = true}
        },
        EnumToJsonSuccessTestParam{
            EnumMessageData{msgs::EnumMessage::TEST_VALUE2},
            R"({"field1":2})",
            {.always_print_enums_as_ints = true}
        },
        EnumToJsonSuccessTestParam{
            EnumMessageData{msgs::EnumMessage::TEST_VALUE2_ALIAS},
            R"({"field1":2})",
            {.always_print_enums_as_ints = true}
        },
        EnumToJsonSuccessTestParam{
            EnumMessageData{msgs::EnumMessage::TEST_VALUE3},
            R"({"field1":3})",
            {.always_print_enums_as_ints = true}
        }
    )
);

TEST_P(EnumToJsonSuccessTest, Test) {
    const auto& param = GetParam();

    auto input = PrepareTestData(param.input);
    formats::json::Value json, expected_json, sample_json;

    UASSERT_NO_THROW((json = MessageToJson(input, param.options)));
    UASSERT_NO_THROW((expected_json = PrepareJsonTestData(param.expected_json)));
    UASSERT_NO_THROW((sample_json = CreateSampleJson(input, param.options)));

    EXPECT_EQ(json, expected_json);
    EXPECT_EQ(expected_json, sample_json);
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
