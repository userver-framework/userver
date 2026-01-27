#include <gtest/gtest.h>

#include <memory>
#include <ostream>
#include <string>

#include <fmt/format.h>
#include <google/protobuf/dynamic_message.h>

#include <userver/formats/json/serialize.hpp>
#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

struct AnyFromJsonSuccessTestParam {
    std::string input = {};
    AnyMessageData expected_message = {};
    ParseOptions options = {};

    // This variable is used to disable some checks that fail in the native protobuf implementation.
    bool skip_native_check = false;
};

struct AnyFromJsonFailureTestParam {
    std::string input = {};
    ParseErrorCode expected_errc = {};
    std::string expected_path = {};
    ParseOptions options = {};
};

void PrintTo(const AnyFromJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

void PrintTo(const AnyFromJsonFailureTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

class AnyFromJsonSuccessTest : public ::testing::TestWithParam<AnyFromJsonSuccessTestParam> {};
class AnyFromJsonFailureTest : public ::testing::TestWithParam<AnyFromJsonFailureTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    AnyFromJsonSuccessTest,
    ::testing::Values(
        AnyFromJsonSuccessTestParam{R"({})", AnyMessageData{}},
        AnyFromJsonSuccessTestParam{R"({"field1":null})", AnyMessageData{}},
        AnyFromJsonSuccessTestParam{R"({"field1":{}})", AnyMessageData{RawAnyData{"", ""}}},
        AnyFromJsonSuccessTestParam{
            R"({
              "field1": {
                "@type":"type.googleapis.com/proto_json.messages.Int32Message"
              }
            })",
            AnyMessageData{Int32MessageData{}}
        },
        AnyFromJsonSuccessTestParam{
            R"({
              "field1": {
                "@type":"type.googleapis.com/proto_json.messages.Int32Message",
                "field1":0,"field2":0,"field3":0
              }
            })",
            AnyMessageData{Int32MessageData{0, 0, 0}}
        },
        AnyFromJsonSuccessTestParam{
            R"({
              "field1": {
                "@type":"type.googleapis.com/proto_json.messages.Int32Message",
                "field1":null,"field2":null,"field3":1
              }
            })",
            AnyMessageData{Int32MessageData{0, 0, 1}}
        },
        AnyFromJsonSuccessTestParam{
            R"({
              "field1": {
                "@type":"type.googleapis.com/proto_json.messages.Int32Message",
                "field1":10,"field2":20,"field3":30
              }
            })",
            AnyMessageData{Int32MessageData{10, 20, 30}}
        },
        AnyFromJsonSuccessTestParam{
            R"({
              "field1": {
                "@type":"type.googleapis.com/proto_json.messages.Int32Message",
                "field1":10,"field2":20,"field3":30,"unknown_field":true
              }
            })",
            AnyMessageData{Int32MessageData{10, 20, 30}},
            {.ignore_unknown_fields = true}
        },
        AnyFromJsonSuccessTestParam{
            R"({
              "field1": {
                "@type":"type.googleapis.com/google.protobuf.Duration",
                "value":"123.000000987s"
              }
            })",
            AnyMessageData{DurationMessageData{123, 987}}
        },
        AnyFromJsonSuccessTestParam{
            R"({
              "field1": {
                "@type":"type.googleapis.com/google.protobuf.Duration",
                "value":"123.000000987s",
                "unknown_field":true
              }
            })",
            AnyMessageData{DurationMessageData{123, 987}},
            {.ignore_unknown_fields = true},
            true  // we want 'ignore_unknown_options' to work in all cases
        },
        AnyFromJsonSuccessTestParam{
            R"({
              "field1": {
                "@type":"type.googleapis.com/google.protobuf.Duration"
              }
            })",
            AnyMessageData{DurationMessageData{0, 0}}
        },
        AnyFromJsonSuccessTestParam{
            R"({
              "field1": {
                "@type":"type.googleapis.com/google.protobuf.Duration",
                "value":null
              }
            })",
            AnyMessageData{DurationMessageData{0, 0}}
        },
        AnyFromJsonSuccessTestParam{
            R"({
              "field1": {
                "@type":"type.googleapis.com/google.protobuf.Value",
                "value":null
              }
            })",
            AnyMessageData{ValueMessageData{kProtoNullValue}}
        },
        AnyFromJsonSuccessTestParam{
            R"({
              "field1": {
                "@type":"type.googleapis.com/google.protobuf.Value",
                "value":"hello"
              }
            })",
            AnyMessageData{ValueMessageData{"hello"}}
        },
        AnyFromJsonSuccessTestParam{
            R"({
              "field1": {
                "@type":"type.googleapis.com/google.protobuf.Value",
                "value":[1.5,1.5]
              }
            })",
            AnyMessageData{ValueMessageData{std::vector<double>{1.5, 1.5}}}
        },
        AnyFromJsonSuccessTestParam{
            R"({
              "field1": {
                "@type":"type.googleapis.com/google.protobuf.Value",
                "value":{"aaa":"hello","bbb":"world"}
              }
            })",
            AnyMessageData{ValueMessageData{std::map<std::string, std::string>{{"aaa", "hello"}, {"bbb", "world"}}}}
        }
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    AnyFromJsonFailureTest,
    ::testing::Values(
        AnyFromJsonFailureTestParam{R"({"field1":[]})", ParseErrorCode::kInvalidType, "field1"},
        AnyFromJsonFailureTestParam{R"({"field1":true})", ParseErrorCode::kInvalidType, "field1"},
        AnyFromJsonFailureTestParam{R"({"field1":1})", ParseErrorCode::kInvalidType, "field1"},
        AnyFromJsonFailureTestParam{R"({"field1":"hello"})", ParseErrorCode::kInvalidType, "field1"},
        AnyFromJsonFailureTestParam{
            R"({
              "field1": {
                "@type":"oops"
              }
            })",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        AnyFromJsonFailureTestParam{
            R"({
              "field1": {
                "@type":"type.googleapis.com/proto_json.messages.NonExistent"
              }
            })",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        AnyFromJsonFailureTestParam{
            R"({
              "field1": {
                "@type":true
              }
            })",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        AnyFromJsonFailureTestParam{
            R"({
              "field1": {
                "value":"123.000000987s"
              }
            })",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        AnyFromJsonFailureTestParam{
            R"({
              "field1": {
                "field1":1
              }
            })",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        AnyFromJsonFailureTestParam{
            R"({
              "field1": {
                "@type":"type.googleapis.com/proto_json.messages.Int32Message",
                "field1":10,"field2":20,"field3":30,"unknown_field":true
              }
            })",
            ParseErrorCode::kUnknownField,
            "field1.unknown_field",
            {.ignore_unknown_fields = false}
        },
        AnyFromJsonFailureTestParam{
            R"({
              "field1": {
                "@type":"type.googleapis.com/google.protobuf.Duration",
                "value":"123.000000987s",
                "unknown_field":true
              }
            })",
            ParseErrorCode::kUnknownField,
            "field1.unknown_field",
            {.ignore_unknown_fields = false}
        }
    )
);

TEST_P(AnyFromJsonSuccessTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::AnyMessage message, expected_message, sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);
    expected_message = PrepareTestData(param.expected_message);

    message.mutable_field1()->set_type_url("dump");

    UASSERT_NO_THROW((message = JsonToMessage<proto_json::messages::AnyMessage>(input, param.options)));

    if (!param.skip_native_check) {
        UASSERT_NO_THROW(InitSampleMessage(param.input, sample_message, param.options));
        CheckMessageEqual(message, sample_message);
    }

    CheckMessageEqual(message, expected_message);
}

TEST_P(AnyFromJsonFailureTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::AnyMessage sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);

    EXPECT_PARSE_ERROR(
        (void)JsonToMessage<proto_json::messages::AnyMessage>(input, param.options),
        param.expected_errc,
        param.expected_path
    );
    UEXPECT_THROW(InitSampleMessage(param.input, sample_message, param.options), SampleError);
}

TEST(AnyFromJsonAdditionalTest, InlinedNonNull) {
    using Message = ::google::protobuf::Any;

    const char* json_str =
        R"({
          "@type":"type.googleapis.com/proto_json.messages.Int32Message",
          "field1":1,"field2":2,"field3":3
        })";
    const auto json = formats::json::FromString(json_str);
    Message message, sample;
    proto_json::messages::Int32Message payload;

    message.set_type_url("dump");

    UASSERT_NO_THROW((message = JsonToMessage<Message>(json)));
    UASSERT_NO_THROW(InitSampleMessage(json_str, sample));
    ASSERT_TRUE(message.Is<proto_json::messages::Int32Message>());
    ASSERT_TRUE(message.UnpackTo(&payload));

    EXPECT_EQ(payload.field1(), 1);
    EXPECT_EQ(payload.field2(), 2);
    EXPECT_EQ(payload.field3(), 3);
    CheckMessageEqual(message, sample);
}

TEST(AnyFromJsonAdditionalTest, InlinedNull) {
    using Message = ::google::protobuf::Any;

    {
        const auto json = formats::json::FromString("null");
        Message message;

        EXPECT_PARSE_ERROR((message = JsonToMessage<Message>(json)), ParseErrorCode::kInvalidType, "/");
        UEXPECT_THROW(InitSampleMessage("null", message), SampleError);
    }

    {
        const auto json = formats::json::FromString("{}");
        Message message, sample;

        message.set_type_url("dump");

        UASSERT_NO_THROW((message = JsonToMessage<Message>(json)));
        UASSERT_NO_THROW(InitSampleMessage("{}", sample));

        EXPECT_TRUE(message.type_url().empty());
        EXPECT_TRUE(message.value().empty());
        CheckMessageEqual(message, sample);
    }
}

TEST(AnyFromJsonAdditionalTest, DynamicMessage) {
    using Message = ::google::protobuf::Any;

    const char* json_str =
        R"({
          "@type":"type.googleapis.com/proto_json.messages.Int32Message",
          "field1":1,"field2":2,"field3":3
        })";
    const auto json = formats::json::FromString(json_str);
    Message message;
    proto_json::messages::Int32Message payload;
    ::google::protobuf::DynamicMessageFactory factory;

    {
        std::unique_ptr<::google::protobuf::Message> message(factory.GetPrototype(Message::descriptor())->New());

        UASSERT_NO_THROW(JsonToMessage(json, *message));

        const auto reflection = message->GetReflection();
        const auto type_url_desc = message->GetDescriptor()->FindFieldByName("type_url");
        const auto value_desc = message->GetDescriptor()->FindFieldByName("value");

        ASSERT_EQ(
            reflection->GetString(*message, type_url_desc),
            "type.googleapis.com/proto_json.messages.Int32Message"
        );
        ASSERT_TRUE(payload.ParseFromString(reflection->GetString(*message, value_desc)));
        EXPECT_EQ(payload.field1(), 1);
        EXPECT_EQ(payload.field2(), 2);
        EXPECT_EQ(payload.field3(), 3);
    }
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
