#include <gtest/gtest.h>

#include <chrono>
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

using namespace std::chrono_literals;

constexpr auto kTestSeconds = 1s + 1min + 1h + 24h + (31 * 24h) + (365 * 24h);

struct TimestampToJsonSuccessTestParam {
    TimestampMessageData input = {};
    std::string expected_json = {};
    PrintOptions options = {};
};

struct TimestampToJsonFailureTestParam {
    TimestampMessageData input = {};
    PrintErrorCode expected_errc = {};
    std::string expected_path = {};
    PrintOptions options = {};

    // Older protobuf versions erroneously pass some checks, we need a way to disable them.
    bool skip_native_check = false;
};

void PrintTo(const TimestampToJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = {{.seconds={}, .nanos={}}} }}", param.input.seconds, param.input.nanos);
}

void PrintTo(const TimestampToJsonFailureTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = {{.seconds={}, .nanos={}}} }}", param.input.seconds, param.input.nanos);
}

class TimestampToJsonSuccessTest : public ::testing::TestWithParam<TimestampToJsonSuccessTestParam> {};
class TimestampToJsonFailureTest : public ::testing::TestWithParam<TimestampToJsonFailureTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    TimestampToJsonSuccessTest,
    ::testing::Values(
        TimestampToJsonSuccessTestParam{TimestampMessageData{0, 0, true}, R"({})"},
        TimestampToJsonSuccessTestParam{TimestampMessageData{0, 0}, R"({"field1": "1970-01-01T00:00:00Z"})"},
        TimestampToJsonSuccessTestParam{
            TimestampMessageData{kTestSeconds.count(), 123456789},
            R"({"field1": "1971-02-02T01:01:01.123456789Z"})"
        },
        TimestampToJsonSuccessTestParam{
            TimestampMessageData{-kTestSeconds.count(), 987654321},
            R"({"field1": "1968-11-29T22:58:59.987654321Z"})"
        },
        TimestampToJsonSuccessTestParam{
            TimestampMessageData{kMaxTimestampSeconds, kMaxTimestampNanos},
            R"({"field1": "9999-12-31T23:59:59.999999999Z"})"
        },
        TimestampToJsonSuccessTestParam{
            TimestampMessageData{kMinTimestampSeconds, kMinTimestampNanos},
            R"({"field1": "0001-01-01T00:00:00Z"})"
        },
        TimestampToJsonSuccessTestParam{
            TimestampMessageData{0, 123456780},
            R"({"field1": "1970-01-01T00:00:00.123456780Z"})"
        },
        TimestampToJsonSuccessTestParam{
            TimestampMessageData{0, 123456000},
            R"({"field1": "1970-01-01T00:00:00.123456Z"})"
        },
        TimestampToJsonSuccessTestParam{
            TimestampMessageData{0, 123400000},
            R"({"field1": "1970-01-01T00:00:00.123400Z"})"
        },
        TimestampToJsonSuccessTestParam{
            TimestampMessageData{0, 100000000},
            R"({"field1": "1970-01-01T00:00:00.100Z"})"
        },
        TimestampToJsonSuccessTestParam{
            TimestampMessageData{1456704000, 123450000},
            R"({"field1": "2016-02-29T00:00:00.123450Z"})"
        },
        TimestampToJsonSuccessTestParam{TimestampMessageData{0, 9}, R"({"field1": "1970-01-01T00:00:00.000000009Z"})"}
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    TimestampToJsonFailureTest,
    ::testing::Values(
        TimestampToJsonFailureTestParam{
            TimestampMessageData{kMaxTimestampSeconds + 1, 0},
            PrintErrorCode::kInvalidValue,
            "field1"
        },
        TimestampToJsonFailureTestParam{
            TimestampMessageData{kMinTimestampSeconds - 1, 0},
            PrintErrorCode::kInvalidValue,
            "field1"
        },
        TimestampToJsonFailureTestParam{
            TimestampMessageData{0, kMaxTimestampNanos + 1},
            PrintErrorCode::kInvalidValue,
            "field1",
            {},
#if GOOGLE_PROTOBUF_VERSION >= 6033000
            false
#else
            true
#endif
        },
        TimestampToJsonFailureTestParam{
            TimestampMessageData{0, kMinTimestampNanos - 1},
            PrintErrorCode::kInvalidValue,
            "field1",
            {},
#if GOOGLE_PROTOBUF_VERSION >= 6033000
            false
#else
            true
#endif
        }
    )
);

TEST_P(TimestampToJsonSuccessTest, Test) {
    const auto& param = GetParam();

    auto input = PrepareTestData(param.input);
    formats::json::Value json, expected_json, sample_json;

    UASSERT_NO_THROW((json = MessageToJson(input, param.options)));
    UASSERT_NO_THROW((expected_json = PrepareJsonTestData(param.expected_json)));
    UASSERT_NO_THROW((sample_json = CreateSampleJson(input, param.options)));

    EXPECT_EQ(json, expected_json);
    EXPECT_EQ(expected_json, sample_json);
}

TEST_P(TimestampToJsonFailureTest, Test) {
    const auto& param = GetParam();
    auto input = PrepareTestData(param.input);

    EXPECT_PRINT_ERROR((void)MessageToJson(input, param.options), param.expected_errc, param.expected_path);

    if (!param.skip_native_check) {
        UEXPECT_THROW((void)CreateSampleJson(input, param.options), SampleError);
    }
}

TEST(TimestampToJsonAdditionalTest, InlinedNonNull) {
    TimestampMessageData data{123, 321};
    auto message = PrepareTestData(data);
    formats::json::Value json, sample;

    UASSERT_NO_THROW((json = MessageToJson(message.field1(), {})));
    UASSERT_NO_THROW((sample = CreateSampleJson(message.field1())));
    ASSERT_TRUE(json.IsString());
    EXPECT_EQ(json, sample);
    EXPECT_EQ(json.As<std::string>(), "1970-01-01T00:02:03.000000321Z");
}

TEST(TimestampToJsonAdditionalTest, InlinedNull) {
    proto_json::messages::TimestampMessage message;
    formats::json::Value json, sample;

    UASSERT_NO_THROW((json = MessageToJson(message.field1(), {})));
    UASSERT_NO_THROW((sample = CreateSampleJson(message.field1())));
    ASSERT_TRUE(json.IsString());
    EXPECT_EQ(json, sample);
    EXPECT_EQ(json.As<std::string>(), "1970-01-01T00:00:00Z");
}

TEST(TimestampToJsonAdditionalTest, DynamicMessage) {
    using Message = ::google::protobuf::Timestamp;
    ::google::protobuf::DynamicMessageFactory factory;

    {
        std::unique_ptr<::google::protobuf::Message> message(factory.GetPrototype(Message::descriptor())->New());
        const auto reflection = message->GetReflection();
        const auto seconds_desc = message->GetDescriptor()->FindFieldByName("seconds");
        const auto nanos_desc = message->GetDescriptor()->FindFieldByName("nanos");

        reflection->SetInt64(message.get(), seconds_desc, 1456704000);
        reflection->SetInt32(message.get(), nanos_desc, 123450000);

        formats::json::Value json;

        UASSERT_NO_THROW((json = MessageToJson(*message, {})));
        ASSERT_TRUE(json.IsString());
        EXPECT_EQ(json.As<std::string>(), "2016-02-29T00:00:00.123450Z");
    }
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
