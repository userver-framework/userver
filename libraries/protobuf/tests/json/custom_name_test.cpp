#include <gtest/gtest.h>

#include <ostream>
#include <string>

#include <fmt/format.h>

#include <userver/formats/json/serialize.hpp>
#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

struct CustomNameToJsonTestParam {
    PrintOptions options = {};
    std::string expected_json = {};
};

struct CustomNameFromJsonTestParam {
    std::string input = {};
};

void PrintTo(const CustomNameToJsonTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ options = {} }}", param.options);
}

void PrintTo(const CustomNameFromJsonTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

class CustomNameToJsonTest : public ::testing::TestWithParam<CustomNameToJsonTestParam> {};
class CustomNameFromJsonTest : public ::testing::TestWithParam<CustomNameFromJsonTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    CustomNameToJsonTest,
    ::testing::Values(
        CustomNameToJsonTestParam{
            {.preserve_proto_field_names = false},
            R"({"___":1001,"_Custom_namE_":1.5,"oneMoreField": "hello","last_field": true})"
        },
        CustomNameToJsonTestParam{
            {.preserve_proto_field_names = true},
            R"({"field":1001,"another_field":1.5,"one_more_field": "hello","last_field": true})"
        }
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    CustomNameFromJsonTest,
    ::testing::Values(
        CustomNameFromJsonTestParam{R"({"___":1001,"_Custom_namE_":1.5,"oneMoreField":"hello","last_field":true})"},
        CustomNameFromJsonTestParam{R"({"field":1001,"another_field":1.5,"one_more_field":"hello","last_field":true})"},
        CustomNameFromJsonTestParam{R"({"field":1001,"_Custom_namE_":1.5,"oneMoreField":"hello","last_field":true})"}
    )
);

TEST_P(CustomNameToJsonTest, Test) {
    using Message = proto_json::messages::CustomNameMessage;
    const auto& param = GetParam();

    Message input;
    input.set_field(1001);
    input.set_another_field(1.5);
    input.set_one_more_field("hello");
    input.set_last_field(true);

    formats::json::Value json, expected_json, sample_json;
    expected_json = formats::json::FromString(param.expected_json);

    UASSERT_NO_THROW((json = MessageToJson(input, param.options)));
    UASSERT_NO_THROW((sample_json = CreateSampleJson(input, param.options)));

    EXPECT_EQ(json, expected_json);
    EXPECT_EQ(expected_json, sample_json);
}

TEST_P(CustomNameFromJsonTest, Test) {
    using Message = proto_json::messages::CustomNameMessage;
    const auto& param = GetParam();

    Message message, sample;
    formats::json::Value input = formats::json::FromString(param.input);

    UASSERT_NO_THROW((message = JsonToMessage<Message>(input)));
    UASSERT_NO_THROW(InitSampleMessage(param.input, sample));

    EXPECT_EQ(message.field(), sample.field());
    EXPECT_EQ(message.another_field(), sample.another_field());
    EXPECT_EQ(message.one_more_field(), sample.one_more_field());
    EXPECT_EQ(message.last_field(), sample.last_field());

    EXPECT_EQ(message.field(), 1001);
    EXPECT_EQ(message.another_field(), 1.5);
    EXPECT_EQ(message.one_more_field(), "hello");
    EXPECT_EQ(message.last_field(), true);
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
