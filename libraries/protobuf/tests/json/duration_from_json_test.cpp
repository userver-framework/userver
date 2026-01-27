#include <gtest/gtest.h>

#include <memory>
#include <ostream>
#include <string>

#include <fmt/format.h>
#include <google/protobuf/dynamic_message.h>

#include <userver/formats/json/serialize.hpp>
#include <userver/protobuf/datetime.hpp>
#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

struct DurationFromJsonSuccessTestParam {
    std::string input = {};
    DurationMessageData expected_message = {};
    ParseOptions options = {};
};

struct DurationFromJsonFailureTestParam {
    std::string input = {};
    ParseErrorCode expected_errc = {};
    std::string expected_path = {};
    ParseOptions options = {};

    // Protobuf ProtoJSON legacy syntax supports some features, which we want to prohibit (because
    // we do not want our clients to use syntax that may break in the newer protobuf versions).
    // This variable is used disable some checks that will fail for legacy syntax.
    bool skip_native_check = false;
};

void PrintTo(const DurationFromJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

void PrintTo(const DurationFromJsonFailureTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

class DurationFromJsonSuccessTest : public ::testing::TestWithParam<DurationFromJsonSuccessTestParam> {};
class DurationFromJsonFailureTest : public ::testing::TestWithParam<DurationFromJsonFailureTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    DurationFromJsonSuccessTest,
    ::testing::Values(
        DurationFromJsonSuccessTestParam{R"({})", DurationMessageData{0, 0, true}},
        DurationFromJsonSuccessTestParam{R"({"field1":null})", DurationMessageData{0, 0, true}},
        DurationFromJsonSuccessTestParam{R"({"field1":"0s"})", DurationMessageData{0, 0}},
        DurationFromJsonSuccessTestParam{R"({"field1":"0.0s"})", DurationMessageData{0, 0}},
        DurationFromJsonSuccessTestParam{R"({"field1":"-0.0s"})", DurationMessageData{0, 0}},
        DurationFromJsonSuccessTestParam{
            R"({"field1":"315576000000.999999999s"})",
            DurationMessageData{kMaxDurationSeconds, kMaxDurationNanos}
        },
        DurationFromJsonSuccessTestParam{
            R"({"field1":"-315576000000.999999999s"})",
            DurationMessageData{kMinDurationSeconds, kMinDurationNanos}
        },
        DurationFromJsonSuccessTestParam{R"({"field1":"123.987654321s"})", DurationMessageData{123, 987654321}},
        DurationFromJsonSuccessTestParam{R"({"field1":"-123.98765432s"})", DurationMessageData{-123, -987654320}},
        DurationFromJsonSuccessTestParam{R"({"field1":"123.9876543s"})", DurationMessageData{123, 987654300}},
        DurationFromJsonSuccessTestParam{R"({"field1":"-123.987654s"})", DurationMessageData{-123, -987654000}},
        DurationFromJsonSuccessTestParam{R"({"field1":"123.987654000s"})", DurationMessageData{123, 987654000}},
        DurationFromJsonSuccessTestParam{R"({"field1":"-123.9s"})", DurationMessageData{-123, -900000000}},
        DurationFromJsonSuccessTestParam{R"({"field1":"123.000000009s"})", DurationMessageData{123, 9}}
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    DurationFromJsonFailureTest,
    ::testing::Values(
        DurationFromJsonFailureTestParam{R"({"field1":[]})", ParseErrorCode::kInvalidType, "field1"},
        DurationFromJsonFailureTestParam{R"({"field1":{}})", ParseErrorCode::kInvalidType, "field1", {}, true},
        DurationFromJsonFailureTestParam{R"({"field1":true})", ParseErrorCode::kInvalidType, "field1"},
        DurationFromJsonFailureTestParam{R"({"field1":1})", ParseErrorCode::kInvalidType, "field1"},
        DurationFromJsonFailureTestParam{R"({"field1":""})", ParseErrorCode::kInvalidValue, "field1"},
        DurationFromJsonFailureTestParam{R"({"field1":"abc"})", ParseErrorCode::kInvalidValue, "field1"},
        DurationFromJsonFailureTestParam{R"({"field1":"0"})", ParseErrorCode::kInvalidValue, "field1"},
        DurationFromJsonFailureTestParam{R"({"field1":"0.-0s"})", ParseErrorCode::kInvalidValue, "field1"},
        DurationFromJsonFailureTestParam{R"({"field1":"0ss"})", ParseErrorCode::kInvalidValue, "field1"},
        DurationFromJsonFailureTestParam{R"({"field1":"123-s"})", ParseErrorCode::kInvalidValue, "field1"},
        DurationFromJsonFailureTestParam{R"({"field1":"-123.-1s"})", ParseErrorCode::kInvalidValue, "field1"},
        DurationFromJsonFailureTestParam{R"({"field1":"315576000001.0s"})", ParseErrorCode::kInvalidValue, "field1"},
        DurationFromJsonFailureTestParam{R"({"field1":"-315576000001.0s"})", ParseErrorCode::kInvalidValue, "field1"},
        DurationFromJsonFailureTestParam{R"({"field1":"0.1000000000s"})", ParseErrorCode::kInvalidValue, "field1"},
        DurationFromJsonFailureTestParam{R"({"field1":"-0.1000000000s"})", ParseErrorCode::kInvalidValue, "field1"},
        DurationFromJsonFailureTestParam{R"({"field1":"1..1s"})", ParseErrorCode::kInvalidValue, "field1"},
        DurationFromJsonFailureTestParam{R"({"field1":".1s"})", ParseErrorCode::kInvalidValue, "field1"},
        DurationFromJsonFailureTestParam{R"({"field1":"1.1.s"})", ParseErrorCode::kInvalidValue, "field1"}
    )
);

TEST_P(DurationFromJsonSuccessTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::DurationMessage message, expected_message, sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);
    expected_message = PrepareTestData(param.expected_message);

    message.mutable_field1()->set_seconds(100001);

    UASSERT_NO_THROW((message = JsonToMessage<proto_json::messages::DurationMessage>(input, param.options)));
    UASSERT_NO_THROW(InitSampleMessage(param.input, sample_message, param.options));

    CheckMessageEqual(message, sample_message);
    CheckMessageEqual(message, expected_message);
}

TEST_P(DurationFromJsonFailureTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::DurationMessage sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);

    EXPECT_PARSE_ERROR(
        (void)JsonToMessage<proto_json::messages::DurationMessage>(input, param.options),
        param.expected_errc,
        param.expected_path
    );

    if (!param.skip_native_check) {
        UEXPECT_THROW(InitSampleMessage(param.input, sample_message, param.options), SampleError);
    }
}

TEST(DurationFromJsonAdditionalTest, InlinedNonNull) {
    using Message = ::google::protobuf::Duration;

    const char* json_str = "\"123.987654321s\"";
    const auto json = formats::json::FromString(json_str);
    Message message, sample;

    message.set_seconds(100001);

    UASSERT_NO_THROW((message = JsonToMessage<Message>(json)));
    UASSERT_NO_THROW(InitSampleMessage(json_str, sample));

    EXPECT_EQ(message.seconds(), 123);
    EXPECT_EQ(message.nanos(), 987654321);
    CheckMessageEqual(message, sample);
}

TEST(DurationFromJsonAdditionalTest, InlinedNull) {
    using Message = ::google::protobuf::Duration;

    const auto json = formats::json::FromString("null");
    Message message, sample;

    message.set_seconds(100001);

    UASSERT_NO_THROW((message = JsonToMessage<Message>(json)));
    UASSERT_NO_THROW(InitSampleMessage("null", sample));

    EXPECT_EQ(message.seconds(), 0);
    EXPECT_EQ(message.nanos(), 0);
    CheckMessageEqual(message, sample);
}

TEST(DurationFromJsonAdditionalTest, DynamicMessage) {
    using Message = ::google::protobuf::Duration;

    const char* json_str = "\"123.000000987s\"";
    const auto json = formats::json::FromString(json_str);
    ::google::protobuf::DynamicMessageFactory factory;

    {
        std::unique_ptr<::google::protobuf::Message> message(factory.GetPrototype(Message::descriptor())->New());

        UASSERT_NO_THROW(JsonToMessage(json, *message));

        const auto reflection = message->GetReflection();
        const auto seconds_desc = message->GetDescriptor()->FindFieldByName("seconds");
        const auto nanos_desc = message->GetDescriptor()->FindFieldByName("nanos");

        EXPECT_EQ(reflection->GetInt64(*message, seconds_desc), 123);
        EXPECT_EQ(reflection->GetInt32(*message, nanos_desc), 987);
    }
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
