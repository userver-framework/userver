#include <gtest/gtest.h>

#include <memory>
#include <string>

#include <fmt/format.h>
#include <google/protobuf/dynamic_message.h>

#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

struct AnyToJsonSuccessTestParam {
    AnyMessageData input = {};
    std::string expected_json = {};
    PrintOptions options = {};

    // This variable is used to disable some checks that fail in the native protobuf implementation.
    bool skip_native_check = false;
};

struct AnyToJsonFailureTestParam {
    AnyMessageData input = {};
    PrintErrorCode expected_errc = {};
    std::string expected_path = {};
    PrintOptions options = {};

    // This variable is used to disable some checks that fail in the native protobuf implementation.
    bool skip_native_check = false;
};

class AnyToJsonSuccessTest : public ::testing::TestWithParam<AnyToJsonSuccessTestParam> {};
class AnyToJsonFailureTest : public ::testing::TestWithParam<AnyToJsonFailureTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    AnyToJsonSuccessTest,
    ::testing::Values(
        AnyToJsonSuccessTestParam{AnyMessageData{}, R"({})"},
        AnyToJsonSuccessTestParam{AnyMessageData{RawAnyData{"", ""}}, R"({"field1":{}})"},
        AnyToJsonSuccessTestParam{
            AnyMessageData{Int32MessageData{}},
            R"({
              "field1": {
                "@type":"type.googleapis.com/proto_json.messages.Int32Message"
              }
            })",
            {},
            true  // native implementation prohibits empty payload for some reason if not in legacy mode
        },
        AnyToJsonSuccessTestParam{
            AnyMessageData{Int32MessageData{}},
            R"({
              "field1": {
                "@type":"type.googleapis.com/proto_json.messages.Int32Message",
                "field1":0,"field2":0,"field3":0
              }
            })",
            {.always_print_fields_with_no_presence = true},
            true
        },
        AnyToJsonSuccessTestParam{
            AnyMessageData{Int32MessageData{10, 20, 30}},
            R"({
              "field1": {
                "@type":"type.googleapis.com/proto_json.messages.Int32Message",
                "field1":10,"field2":20,"field3":30
              }
            })"
        },
        AnyToJsonSuccessTestParam{
            AnyMessageData{DurationMessageData{123, 987}},
            R"({
              "field1": {
                "@type":"type.googleapis.com/google.protobuf.Duration",
                "value":"123.000000987s"
              }
            })"
        },
        AnyToJsonSuccessTestParam{
            AnyMessageData{ValueMessageData{kProtoNullValue}},
            R"({
              "field1": {
                "@type":"type.googleapis.com/google.protobuf.Value",
                "value":null
              }
            })"
        },
        AnyToJsonSuccessTestParam{
            AnyMessageData{ValueMessageData{1.5}},
            R"({
              "field1": {
                "@type":"type.googleapis.com/google.protobuf.Value",
                "value":1.5
              }
            })"
        },
        AnyToJsonSuccessTestParam{
            AnyMessageData{ValueMessageData{std::vector<double>{1.5, 1.5}}},
            R"({
              "field1": {
                "@type":"type.googleapis.com/google.protobuf.Value",
                "value":[1.5,1.5]
              }
            })"
        },
        AnyToJsonSuccessTestParam{
            AnyMessageData{ValueMessageData{std::map<std::string, std::string>{{"aaa", "hello"}}}},
            R"({
              "field1": {
                "@type":"type.googleapis.com/google.protobuf.Value",
                "value":{"aaa":"hello"}
              }
            })"
        },
        AnyToJsonSuccessTestParam{
            AnyMessageData{Int32MessageData{100, 200, 300}, true},
            R"({
              "field1": {
                "@type":"type.googleapis.com/google.protobuf.Any",
                "value": {
                  "@type":"type.googleapis.com/proto_json.messages.Int32Message",
                  "field1":100,"field2":200,"field3":300
                }
              }
            })"
        },
        AnyToJsonSuccessTestParam{
            AnyMessageData{DurationMessageData{-123, -567}, true},
            R"({
              "field1": {
                "@type":"type.googleapis.com/google.protobuf.Any",
                "value": {
                  "@type":"type.googleapis.com/google.protobuf.Duration",
                  "value":"-123.000000567s"
                }
              }
            })"
        },
        AnyToJsonSuccessTestParam{
            AnyMessageData{ValueMessageData{true}, true},
            R"({
              "field1": {
                "@type":"type.googleapis.com/google.protobuf.Any",
                "value": {
                  "@type":"type.googleapis.com/google.protobuf.Value",
                  "value":true
                }
              }
            })"
        }
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    AnyToJsonFailureTest,
    ::testing::Values(
        AnyToJsonFailureTestParam{AnyMessageData{RawAnyData{"", "\x08\x01"}}, PrintErrorCode::kInvalidValue, "field1"},
        AnyToJsonFailureTestParam{AnyMessageData{RawAnyData{"oops", ""}}, PrintErrorCode::kInvalidValue, "field1"},
        AnyToJsonFailureTestParam{
            AnyMessageData{RawAnyData{"type.googleapis.com/proto_json.messages.NonExistent", "\x08\x01"}},
            PrintErrorCode::kInvalidValue,
            "field1"
        },
        // '\x80' is incomplete message
        AnyToJsonFailureTestParam{
            AnyMessageData{RawAnyData{"type.googleapis.com/proto_json.messages.Int32Message", "\x80"}},
            PrintErrorCode::kInvalidValue,
            "field1",
            {},
            true  // native implementation does not fail on invalid binary data and produces strange JSON
        },
        AnyToJsonFailureTestParam{AnyMessageData{DurationMessageData{1, -1}}, PrintErrorCode::kInvalidValue, "field1"},
        AnyToJsonFailureTestParam{
            AnyMessageData{ValueMessageData{std::vector<double>{std::numeric_limits<double>::infinity()}}},
            PrintErrorCode::kInvalidValue,
            "field1.list_value.values[0].number_value"
        }
    )
);

TEST_P(AnyToJsonSuccessTest, Test) {
    const auto& param = GetParam();

    auto input = PrepareTestData(param.input);
    formats::json::Value json, expected_json, sample_json;

    UASSERT_NO_THROW((json = MessageToJson(input, param.options)));
    UASSERT_NO_THROW((expected_json = PrepareJsonTestData(param.expected_json)));

    if (!param.skip_native_check) {
        UASSERT_NO_THROW((sample_json = CreateSampleJson(input, param.options)));
        EXPECT_EQ(json, sample_json);
    }

    EXPECT_EQ(json, expected_json);
}

TEST_P(AnyToJsonFailureTest, Test) {
    const auto& param = GetParam();
    auto input = PrepareTestData(param.input);

    EXPECT_PRINT_ERROR((void)MessageToJson(input, param.options), param.expected_errc, param.expected_path);

    if (!param.skip_native_check) {
        UEXPECT_THROW((void)CreateSampleJson(input, param.options), SampleError);
    }
}

TEST(AnyToJsonAdditionalTest, InlinedNonNull) {
    AnyMessageData data{Int32MessageData{1, 2, 3}};
    auto message = PrepareTestData(data);
    formats::json::Value json, sample;

    UASSERT_NO_THROW((json = MessageToJson(message.field1(), {})));
    UASSERT_NO_THROW((sample = CreateSampleJson(message.field1())));
    ASSERT_TRUE(json.IsObject());
    EXPECT_EQ(json, sample);
    EXPECT_EQ(json["@type"].As<std::string>(), "type.googleapis.com/proto_json.messages.Int32Message");
    EXPECT_EQ(json["field1"].As<std::int32_t>(), 1);
    EXPECT_EQ(json["field2"].As<std::int32_t>(), 2);
    EXPECT_EQ(json["field3"].As<std::int32_t>(), 3);
}

TEST(AnyToJsonAdditionalTest, InlinedNull) {
    proto_json::messages::AnyMessage message;
    formats::json::Value json, sample;

    UASSERT_NO_THROW((json = MessageToJson(message.field1(), {})));
    UASSERT_NO_THROW((sample = CreateSampleJson(message.field1())));
    ASSERT_TRUE(json.IsObject());
    EXPECT_EQ(json, sample);
    EXPECT_EQ(json.GetSize(), std::size_t{0});
}

TEST(AnyToJsonAdditionalTest, DynamicMessage) {
    using Message = ::google::protobuf::Any;
    using ProtobufStringType =
        decltype(std::declval<::google::protobuf::Reflection>()
                     .GetString(std::declval<const ::google::protobuf::Message&>(), nullptr));

    ::google::protobuf::DynamicMessageFactory factory;
    proto_json::messages::Int32Message payload;
    ProtobufStringType payload_str;

    payload.set_field1(1001);
    payload.set_field2(1002);
    payload.set_field3(1003);

    ASSERT_TRUE(payload.SerializeToString(&payload_str));
    proto_json::messages::Int32Message payload2;

    {
        std::unique_ptr<::google::protobuf::Message> message(factory.GetPrototype(Message::descriptor())->New());
        const auto reflection = message->GetReflection();
        const auto type_url_desc = message->GetDescriptor()->FindFieldByName("type_url");
        const auto value_desc = message->GetDescriptor()->FindFieldByName("value");

        reflection->SetString(message.get(), type_url_desc, "type.googleapis.com/proto_json.messages.Int32Message");
        reflection->SetString(message.get(), value_desc, payload_str);

        formats::json::Value json;

        UASSERT_NO_THROW((json = MessageToJson(*message, {})));
        ASSERT_TRUE(json.IsObject());
        ASSERT_EQ(json.GetSize(), std::size_t{4});
        EXPECT_EQ(json["@type"].As<std::string>(), "type.googleapis.com/proto_json.messages.Int32Message");
        EXPECT_EQ(json["field1"].As<std::int32_t>(), 1001);
        EXPECT_EQ(json["field2"].As<std::int32_t>(), 1002);
        EXPECT_EQ(json["field3"].As<std::int32_t>(), 1003);
    }
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
