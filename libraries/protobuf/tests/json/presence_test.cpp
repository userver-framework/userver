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

struct PresenceToJsonTestParam {
    PresenceMessageData input = {};
    std::string expected_json = {};
    PrintOptions options = {};
};

struct PresenceFromJsonTestParam {
    std::string input = {};
    PresenceMessageData expected_message = {};
};

void PrintTo(const PresenceFromJsonTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

class PresenceToJsonTest : public ::testing::TestWithParam<PresenceToJsonTestParam> {};
class PresenceFromJsonTest : public ::testing::TestWithParam<PresenceFromJsonTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    PresenceToJsonTest,
    ::testing::Values(
        PresenceToJsonTestParam{PresenceMessageData{}, R"({})"},
        PresenceToJsonTestParam{
            PresenceMessageData{},
            R"({"field1":false,"field4":[],"field5":{}})",
            {.always_print_fields_with_no_presence = true}
        },
        PresenceToJsonTestParam{
            PresenceMessageData{true, false, Int32MessageData{1, 2, 3}, {false, false}, {{1, false}, {2, false}}},
            R"({
              "field1":true,
              "field2":false,
              "field3":{"field1":1,"field2":2,"field3":3},
              "field4":[false,false],
              "field5":{"1":false,"2":false}
            })"
        },
        PresenceToJsonTestParam{
            PresenceMessageData{true, false, Int32MessageData{1, 2, 3}, {false, false}, {{1, false}, {2, false}}},
            R"({
              "field1":true,
              "field2":false,
              "field3":{"field1":1,"field2":2,"field3":3},
              "field4":[false,false],
              "field5":{"1":false,"2":false}
            })",
            {.always_print_fields_with_no_presence = true}
        },
        PresenceToJsonTestParam{
            PresenceMessageData{false, std::nullopt, Int32MessageData{0, 0, 0}, {true}, {}},
            R"({
              "field3":{},
              "field4":[true]
            })"
        },
        PresenceToJsonTestParam{
            PresenceMessageData{false, std::nullopt, Int32MessageData{0, 0, 0}, {true}, {}},
            R"({
              "field1":false,
              "field3":{"field1":0,"field2":0,"field3":0},
              "field4":[true],
              "field5":{}
            })",
            {.always_print_fields_with_no_presence = true}
        }
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    PresenceFromJsonTest,
    ::testing::Values(
        PresenceFromJsonTestParam{R"({})", PresenceMessageData{}},
        PresenceFromJsonTestParam{R"({"field1":false,"field2":false})", PresenceMessageData{.field2 = false}},
        PresenceFromJsonTestParam{
            R"({"field1":false,"field2":false,"field3":{}})",
            PresenceMessageData{.field2 = false, .field3 = Int32MessageData{}}
        },
        PresenceFromJsonTestParam{
            R"({"field1":false,"field2":false,"field3":{"field1":0,"field2":0,"field3":0}})",
            PresenceMessageData{.field2 = false, .field3 = Int32MessageData{}}
        },
        PresenceFromJsonTestParam{
            R"({
              "field1":true,
              "field2":false,
              "field3":{"field1":1,"field2":2,"field3":3},
              "field4":[false],
              "field5":{"1":false}})",
            PresenceMessageData{true, false, Int32MessageData{1, 2, 3}, {false}, {{1, false}}}
        }
    )
);

TEST_P(PresenceToJsonTest, Test) {
    const auto& param = GetParam();

    auto input = PrepareTestData(param.input);
    formats::json::Value json, expected_json, sample_json;

    UASSERT_NO_THROW((json = MessageToJson(input, param.options)));
    UASSERT_NO_THROW((expected_json = PrepareJsonTestData(param.expected_json)));
    UASSERT_NO_THROW((sample_json = CreateSampleJson(input, param.options)));

    EXPECT_EQ(json, expected_json);
    EXPECT_EQ(expected_json, sample_json);
}

TEST_P(PresenceFromJsonTest, Test) {
    using Message = proto_json::messages::PresenceMessage;
    const auto& param = GetParam();

    Message message, expected_message, sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);
    expected_message = PrepareTestData(param.expected_message);

    UASSERT_NO_THROW((message = JsonToMessage<Message>(input)));
    UASSERT_NO_THROW(InitSampleMessage(param.input, sample_message));

    CheckMessageEqual(message, sample_message);
    CheckMessageEqual(message, expected_message);
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
