#include <gtest/gtest.h>

#include <memory>
#include <ostream>
#include <string>

#include <fmt/format.h>
#include <google/protobuf/dynamic_message.h>

#include <userver/protobuf/datetime.hpp>
#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

struct DurationToJsonSuccessTestParam {
    DurationMessageData input = {};
    std::string expected_json = {};
    PrintOptions options = {};
};

struct DurationToJsonFailureTestParam {
    DurationMessageData input = {};
    PrintErrorCode expected_errc = {};
    std::string expected_path = {};
    PrintOptions options = {};
};

void PrintTo(const DurationToJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = {{.seconds={}, .nanos={}}} }}", param.input.seconds, param.input.nanos);
}

void PrintTo(const DurationToJsonFailureTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = {{.seconds={}, .nanos={}}} }}", param.input.seconds, param.input.nanos);
}

class DurationToJsonSuccessTest : public ::testing::TestWithParam<DurationToJsonSuccessTestParam> {};
class DurationToJsonFailureTest : public ::testing::TestWithParam<DurationToJsonFailureTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    DurationToJsonSuccessTest,
    ::testing::Values(
        DurationToJsonSuccessTestParam{DurationMessageData{0, 0, true}, R"({})"},
        DurationToJsonSuccessTestParam{DurationMessageData{0, 0}, R"({"field1": "0s"})"},
        DurationToJsonSuccessTestParam{DurationMessageData{123, 0}, R"({"field1": "123s"})"},
        DurationToJsonSuccessTestParam{DurationMessageData{-123, 0}, R"({"field1": "-123s"})"},
        DurationToJsonSuccessTestParam{DurationMessageData{0, 567}, R"({"field1": "0.000000567s"})"},
        DurationToJsonSuccessTestParam{DurationMessageData{0, -567}, R"({"field1": "-0.000000567s"})"},
        DurationToJsonSuccessTestParam{DurationMessageData{123, 987654321}, R"({"field1": "123.987654321s"})"},
        DurationToJsonSuccessTestParam{DurationMessageData{-123, -987654320}, R"({"field1": "-123.987654320s"})"},
        DurationToJsonSuccessTestParam{DurationMessageData{123, 987654000}, R"({"field1": "123.987654s"})"},
        DurationToJsonSuccessTestParam{DurationMessageData{-123, -987600000}, R"({"field1": "-123.987600s"})"},
        DurationToJsonSuccessTestParam{DurationMessageData{123, 987000000}, R"({"field1": "123.987s"})"},
        DurationToJsonSuccessTestParam{DurationMessageData{-123, -980000000}, R"({"field1": "-123.980s"})"},
        DurationToJsonSuccessTestParam{
            DurationMessageData{kMaxDurationSeconds, kMaxDurationNanos},
            R"({"field1": "315576000000.999999999s"})"
        },
        DurationToJsonSuccessTestParam{
            DurationMessageData{kMinDurationSeconds, kMinDurationNanos},
            R"({"field1": "-315576000000.999999999s"})"
        }
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    DurationToJsonFailureTest,
    ::testing::Values(
        DurationToJsonFailureTestParam{
            DurationMessageData{kMaxDurationSeconds + 1, 0},
            PrintErrorCode::kInvalidValue,
            "field1"
        },
        DurationToJsonFailureTestParam{
            DurationMessageData{kMinDurationSeconds - 1, 0},
            PrintErrorCode::kInvalidValue,
            "field1"
        },
        DurationToJsonFailureTestParam{
            DurationMessageData{0, kMaxDurationNanos + 1},
            PrintErrorCode::kInvalidValue,
            "field1"
        },
        DurationToJsonFailureTestParam{
            DurationMessageData{0, kMinDurationNanos - 1},
            PrintErrorCode::kInvalidValue,
            "field1"
        },
        DurationToJsonFailureTestParam{DurationMessageData{1, -1}, PrintErrorCode::kInvalidValue, "field1"}
    )
);

TEST_P(DurationToJsonSuccessTest, Test) {
    const auto& param = GetParam();

    auto input = PrepareTestData(param.input);
    formats::json::Value json, expected_json, sample_json;

    UASSERT_NO_THROW((json = MessageToJson(input, param.options)));
    UASSERT_NO_THROW((expected_json = PrepareJsonTestData(param.expected_json)));
    UASSERT_NO_THROW((sample_json = CreateSampleJson(input, param.options)));

    EXPECT_EQ(json, expected_json);
    EXPECT_EQ(expected_json, sample_json);
}

TEST_P(DurationToJsonFailureTest, Test) {
    const auto& param = GetParam();
    auto input = PrepareTestData(param.input);

    EXPECT_PRINT_ERROR((void)MessageToJson(input, param.options), param.expected_errc, param.expected_path);
    UEXPECT_THROW((void)CreateSampleJson(input, param.options), SampleError);
}

TEST(DurationToJsonAdditionalTest, InlinedNonNull) {
    DurationMessageData data{123, 321};
    auto message = PrepareTestData(data);
    formats::json::Value json, sample;

    UASSERT_NO_THROW((json = MessageToJson(message.field1(), {})));
    UASSERT_NO_THROW((sample = CreateSampleJson(message.field1())));
    ASSERT_TRUE(json.IsString());
    EXPECT_EQ(json, sample);
    EXPECT_EQ(json.As<std::string>(), "123.000000321s");
}

TEST(DurationToJsonAdditionalTest, InlinedNull) {
    proto_json::messages::DurationMessage message;
    formats::json::Value json, sample;

    UASSERT_NO_THROW((json = MessageToJson(message.field1(), {})));
    UASSERT_NO_THROW((sample = CreateSampleJson(message.field1())));
    ASSERT_TRUE(json.IsString());
    EXPECT_EQ(json, sample);
    EXPECT_EQ(json.As<std::string>(), "0s");
}

TEST(DurationToJsonAdditionalTest, DynamicMessage) {
    using Message = ::google::protobuf::Duration;
    ::google::protobuf::DynamicMessageFactory factory;

    {
        std::unique_ptr<::google::protobuf::Message> message(factory.GetPrototype(Message::descriptor())->New());
        const auto reflection = message->GetReflection();
        const auto seconds_desc = message->GetDescriptor()->FindFieldByName("seconds");
        const auto nanos_desc = message->GetDescriptor()->FindFieldByName("nanos");

        reflection->SetInt64(message.get(), seconds_desc, -123);
        reflection->SetInt32(message.get(), nanos_desc, -987);

        formats::json::Value json;

        UASSERT_NO_THROW((json = MessageToJson(*message, {})));
        ASSERT_TRUE(json.IsString());
        EXPECT_EQ(json.As<std::string>(), "-123.000000987s");
    }
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
