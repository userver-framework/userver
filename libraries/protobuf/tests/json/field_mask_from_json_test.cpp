#include <gtest/gtest.h>

#include <memory>
#include <ostream>
#include <string>
#include <unordered_set>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <google/protobuf/dynamic_message.h>

#include <userver/formats/json/serialize.hpp>
#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

struct FieldMaskFromJsonSuccessTestParam {
    std::string input = {};
    FieldMaskMessageData expected_message = {};
    ParseOptions options = {};
};

struct FieldMaskFromJsonFailureTestParam {
    std::string input = {};
    ParseErrorCode expected_errc = {};
    std::string expected_path = {};
    ParseOptions options = {};

    // Protobuf ProtoJSON legacy syntax supports some features, which we want to prohibit (because
    // we do not want our clients to use syntax that may break in the newer protobuf versions).
    // This variable is used disable some checks that will fail for legacy syntax.
    bool skip_native_check = false;
};

void PrintTo(const FieldMaskFromJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

void PrintTo(const FieldMaskFromJsonFailureTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

class FieldMaskFromJsonSuccessTest : public ::testing::TestWithParam<FieldMaskFromJsonSuccessTestParam> {};
class FieldMaskFromJsonFailureTest : public ::testing::TestWithParam<FieldMaskFromJsonFailureTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    FieldMaskFromJsonSuccessTest,
    ::testing::Values(
        FieldMaskFromJsonSuccessTestParam{R"({})", FieldMaskMessageData{}},
        FieldMaskFromJsonSuccessTestParam{R"({"field1":null})", FieldMaskMessageData{}},
        FieldMaskFromJsonSuccessTestParam{R"({"field1":""})", FieldMaskMessageData{std::vector<std::string>{}}},
        FieldMaskFromJsonSuccessTestParam{R"({"field1":","})", FieldMaskMessageData{std::vector<std::string>{"", ""}}},
        FieldMaskFromJsonSuccessTestParam{
            R"({"field1":",,,aaa,"})",
            FieldMaskMessageData{std::vector<std::string>{"", "", "", "aaa", ""}}
        },
        FieldMaskFromJsonSuccessTestParam{
            R"({"field1":"someField.anotherField"})",
            FieldMaskMessageData{std::vector<std::string>{"some_field.another_field"}}
        },
        FieldMaskFromJsonSuccessTestParam{
            R"({"field1":"someField.anotherField..oneMore,AB0C"})",
            FieldMaskMessageData{std::vector<std::string>{"some_field.another_field..one_more", "_a_b0_c"}}
        }
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    FieldMaskFromJsonFailureTest,
    ::testing::Values(
        FieldMaskFromJsonFailureTestParam{R"({"field1":[]})", ParseErrorCode::kInvalidType, "field1"},
        FieldMaskFromJsonFailureTestParam{R"({"field1":{}})", ParseErrorCode::kInvalidType, "field1", {}, true},
        FieldMaskFromJsonFailureTestParam{R"({"field1":true})", ParseErrorCode::kInvalidType, "field1"},
        FieldMaskFromJsonFailureTestParam{R"({"field1":1})", ParseErrorCode::kInvalidType, "field1"},
        FieldMaskFromJsonFailureTestParam{
            R"({"field1":"some_field"})",
            ParseErrorCode::kInvalidValue,
            "field1",
            {},
            true
        },
        FieldMaskFromJsonFailureTestParam{
            R"({"field1":"someF!ield"})",
            ParseErrorCode::kInvalidValue,
            "field1",
            {},
            true
        }
    )
);

TEST_P(FieldMaskFromJsonSuccessTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::FieldMaskMessage message, expected_message, sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);
    expected_message = PrepareTestData(param.expected_message);

    message.mutable_field1()->add_paths("dump");

    UASSERT_NO_THROW((message = JsonToMessage<proto_json::messages::FieldMaskMessage>(input, param.options)));
    UASSERT_NO_THROW(InitSampleMessage(param.input, sample_message, param.options));

    CheckMessageEqual(message, sample_message);
    CheckMessageEqual(message, expected_message);
}

TEST_P(FieldMaskFromJsonFailureTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::FieldMaskMessage sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);

    EXPECT_PARSE_ERROR(
        (void)JsonToMessage<proto_json::messages::FieldMaskMessage>(input, param.options),
        param.expected_errc,
        param.expected_path
    );

    if (!param.skip_native_check) {
        UEXPECT_THROW(InitSampleMessage(param.input, sample_message, param.options), SampleError);
    }
}

TEST(FieldMaskFromJsonAdditionalTest, InlinedNonNull) {
    using Message = ::google::protobuf::FieldMask;

    const char* json_str = "\"someField.anotherField,oneMore\"";
    const auto json = formats::json::FromString(json_str);
    Message message, sample;

    message.add_paths("dump");

    UASSERT_NO_THROW((message = JsonToMessage<Message>(json)));
    UASSERT_NO_THROW(InitSampleMessage(json_str, sample));

    CheckMessageEqual(message, sample);
}

TEST(FieldMaskFromJsonAdditionalTest, InlinedNull) {
    using Message = ::google::protobuf::FieldMask;

    {
        const auto json = formats::json::FromString("null");
        Message message;

        EXPECT_PARSE_ERROR((message = JsonToMessage<Message>(json)), ParseErrorCode::kInvalidType, "/");
        UEXPECT_THROW(InitSampleMessage("null", message), SampleError);
    }

    {
        const auto json = formats::json::FromString("\"\"");
        Message message, sample;

        message.add_paths("dump");

        UASSERT_NO_THROW((message = JsonToMessage<Message>(json)));
        UASSERT_NO_THROW(InitSampleMessage("{}", sample));

        EXPECT_TRUE(message.paths().empty());
        CheckMessageEqual(message, sample);
    }
}

TEST(FieldMaskFromJsonAdditionalTest, DynamicMessage) {
    using Message = ::google::protobuf::FieldMask;

    const char* json_str = "\"someField.anotherField,oneMore\"";
    const auto json = formats::json::FromString(json_str);
    ::google::protobuf::DynamicMessageFactory factory;

    {
        std::unique_ptr<::google::protobuf::Message> message(factory.GetPrototype(Message::descriptor())->New());

        UASSERT_NO_THROW(JsonToMessage(json, *message));

        const auto reflection = message->GetReflection();
        const auto paths_desc = message->GetDescriptor()->FindFieldByName("paths");
        std::unordered_multiset<std::string> paths;

        for (int i = 0; i < reflection->FieldSize(*message, paths_desc); ++i) {
            paths.insert(reflection->GetRepeatedString(*message, paths_desc, i));
        }

        EXPECT_EQ(paths, (std::unordered_multiset<std::string>{"some_field.another_field", "one_more"}));
    }
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
