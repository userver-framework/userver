#include <gtest/gtest.h>

#include <chrono>
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

using namespace std::chrono_literals;

constexpr auto kTestSeconds = 1s + 1min + 1h + 24h + (31 * 24h) + (365 * 24h);

struct TimestampFromJsonSuccessTestParam {
    std::string input = {};
    TimestampMessageData expected_message = {};
    ParseOptions options = {};
};

struct TimestampFromJsonFailureTestParam {
    std::string input = {};
    ParseErrorCode expected_errc = {};
    std::string expected_path = {};
    ParseOptions options = {};

    // Protobuf ProtoJSON legacy syntax supports some features, which we want to prohibit (because
    // we do not want our clients to use syntax that may break in the newer protobuf versions).
    // This variable is used disable some checks that will fail for legacy syntax.
    bool skip_native_check = false;
};

void PrintTo(const TimestampFromJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

void PrintTo(const TimestampFromJsonFailureTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

class TimestampFromJsonSuccessTest : public ::testing::TestWithParam<TimestampFromJsonSuccessTestParam> {};
class TimestampFromJsonFailureTest : public ::testing::TestWithParam<TimestampFromJsonFailureTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    TimestampFromJsonSuccessTest,
    ::testing::Values(
        TimestampFromJsonSuccessTestParam{R"({})", TimestampMessageData{0, 0, true}},
        TimestampFromJsonSuccessTestParam{R"({"field1":null})", TimestampMessageData{0, 0, true}},
        TimestampFromJsonSuccessTestParam{R"({"field1":"1970-01-01T00:00:00Z"})", TimestampMessageData{0, 0}},
        TimestampFromJsonSuccessTestParam{
            R"({"field1":"1971-02-02T01:01:01.123456789Z"})",
            TimestampMessageData{kTestSeconds.count(), 123456789}
        },
        TimestampFromJsonSuccessTestParam{
            R"({"field1":"1968-11-29T22:58:59.987654321Z"})",
            TimestampMessageData{-kTestSeconds.count(), 987654321}
        },
        TimestampFromJsonSuccessTestParam{
            R"({"field1":"9999-12-31T23:59:59.999999999Z"})",
            TimestampMessageData{kMaxTimestampSeconds, kMaxTimestampNanos}
        },
        TimestampFromJsonSuccessTestParam{
            R"({"field1":"0001-01-01T00:00:00Z"})",
            TimestampMessageData{kMinTimestampSeconds, kMinTimestampNanos}
        },
        TimestampFromJsonSuccessTestParam{
            R"({"field1":"1970-01-01T00:00:00.123456780Z"})",
            TimestampMessageData{0, 123456780}
        },
        TimestampFromJsonSuccessTestParam{
            R"({"field1":"1970-01-01T00:00:00.123456Z"})",
            TimestampMessageData{0, 123456000}
        },
        TimestampFromJsonSuccessTestParam{
            R"({"field1":"1970-01-01T00:00:00.123400Z"})",
            TimestampMessageData{0, 123400000}
        },
        TimestampFromJsonSuccessTestParam{
            R"({"field1":"1970-01-01T00:00:00.100Z"})",
            TimestampMessageData{0, 100000000}
        },
        TimestampFromJsonSuccessTestParam{R"({"field1":"1970-01-01T00:00:00.000000009Z"})", TimestampMessageData{0, 9}},
        TimestampFromJsonSuccessTestParam{
            R"({"field1":"1970-01-01T00:01:13.123+02:00"})",
            TimestampMessageData{-(2 * 60 * 60) + 60 + 13, 123000000}
        },
        TimestampFromJsonSuccessTestParam{
            R"({"field1":"1970-01-01T00:01:13.123-02:00"})",
            TimestampMessageData{(2 * 60 * 60) + 60 + 13, 123000000}
        },
        TimestampFromJsonSuccessTestParam{
            R"({"field1":"1970-01-01T00:01:13.123+02:10"})",
            TimestampMessageData{-(2 * 60 * 60 + 10 * 60) + 60 + 13, 123000000}
        },
        TimestampFromJsonSuccessTestParam{
            R"({"field1":"1970-01-01T00:01:13.123-02:10"})",
            TimestampMessageData{(2 * 60 * 60 + 10 * 60) + 60 + 13, 123000000}
        },
        TimestampFromJsonSuccessTestParam{
            R"({"field1":"1970-01-01T00:01:13.123+00:00"})",
            TimestampMessageData{60 + 13, 123000000}
        },
        TimestampFromJsonSuccessTestParam{
            R"({"field1":"1970-01-01T00:01:13.123-00:00"})",
            TimestampMessageData{60 + 13, 123000000}
        }
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    TimestampFromJsonFailureTest,
    ::testing::Values(
        TimestampFromJsonFailureTestParam{R"({"field1":[]})", ParseErrorCode::kInvalidType, "field1"},
        TimestampFromJsonFailureTestParam{R"({"field1":{}})", ParseErrorCode::kInvalidType, "field1", {}, true},
        TimestampFromJsonFailureTestParam{R"({"field1":true})", ParseErrorCode::kInvalidType, "field1"},
        TimestampFromJsonFailureTestParam{R"({"field1":10})", ParseErrorCode::kInvalidType, "field1"},
        TimestampFromJsonFailureTestParam{R"({"field1":""})", ParseErrorCode::kInvalidValue, "field1"},
        TimestampFromJsonFailureTestParam{R"({"field1":"abc"})", ParseErrorCode::kInvalidValue, "field1"},
        TimestampFromJsonFailureTestParam{R"({"field1":"-"})", ParseErrorCode::kInvalidValue, "field1"},
        TimestampFromJsonFailureTestParam{R"({"field1":"2010"})", ParseErrorCode::kInvalidValue, "field1"},
        TimestampFromJsonFailureTestParam{R"({"field1":"2010-"})", ParseErrorCode::kInvalidValue, "field1"},
        TimestampFromJsonFailureTestParam{R"({"field1":"2010-10"})", ParseErrorCode::kInvalidValue, "field1"},
        TimestampFromJsonFailureTestParam{R"({"field1":"2010-10-"})", ParseErrorCode::kInvalidValue, "field1"},
        TimestampFromJsonFailureTestParam{R"({"field1":"2010-10-10"})", ParseErrorCode::kInvalidValue, "field1"},
        TimestampFromJsonFailureTestParam{R"({"field1":"2010-10-10T"})", ParseErrorCode::kInvalidValue, "field1"},
        TimestampFromJsonFailureTestParam{R"({"field1":"2010-10-10T10"})", ParseErrorCode::kInvalidValue, "field1"},
        TimestampFromJsonFailureTestParam{R"({"field1":"2010-10-10T10:10"})", ParseErrorCode::kInvalidValue, "field1"},
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010-10-10T10:10:10"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010-10-10T10:10:10z"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010-10-10T10:10:10ZZ"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010-10-10T10:10:10+"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010-10-10T10:10:10+02"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010-10-10T10:10:10-02:"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010-10-10T10:10:10+02:00Z"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010-10-10T10:10:10+02:0"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010-10-10T10:10:10+2:00"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010-10-10T10:10:10Z+02:00"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010-10-10T10:10:1Z"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010-10-10T10:1:10Z"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010-10-10T1:10:10Z"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010-10-10T1:10:10Z"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010-10-10t10:10:10Z"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010-10-10TT10:10:10Z"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010-10-1T10:10:10Z"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010-1-10T10:10:10Z"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"201-10-10T10:10:10Z"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"201A-10-10T10:10:10Z"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010--10-10T10:10:10Z"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010-1A-10T10:10:10Z"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010-10--10T10:10:10Z"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010-10-1AT10:10:10Z"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010-10-10T1A:10:10Z"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010-10-10T10-10:10Z"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010-10-10T10:1A:10Z"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010-10-10T10:10::10Z"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010-10-10T10:10:1AZ"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        },
        TimestampFromJsonFailureTestParam{
            R"({"field1":"2010-10-10T10:10:10+-02:00"})",
            ParseErrorCode::kInvalidValue,
            "field1"
        }
    )
);

TEST_P(TimestampFromJsonSuccessTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::TimestampMessage message, expected_message, sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);
    expected_message = PrepareTestData(param.expected_message);

    message.mutable_field1()->set_seconds(100001);

    UASSERT_NO_THROW((message = JsonToMessage<proto_json::messages::TimestampMessage>(input, param.options)));
    UASSERT_NO_THROW(InitSampleMessage(param.input, sample_message, param.options));

    CheckMessageEqual(message, sample_message);
    CheckMessageEqual(message, expected_message);
}

TEST_P(TimestampFromJsonFailureTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::TimestampMessage sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);

    EXPECT_PARSE_ERROR(
        (void)JsonToMessage<proto_json::messages::TimestampMessage>(input, param.options),
        param.expected_errc,
        param.expected_path
    );

    if (!param.skip_native_check) {
        UEXPECT_THROW(InitSampleMessage(param.input, sample_message, param.options), SampleError);
    }
}

TEST(TimestampFromJsonAdditionalTest, InlinedNonNull) {
    using Message = ::google::protobuf::Timestamp;

    const char* json_str = "\"1970-01-01T00:02:03.000000321Z\"";
    const auto json = formats::json::FromString(json_str);
    Message message, sample;

    message.set_seconds(100001);

    UASSERT_NO_THROW((message = JsonToMessage<Message>(json)));
    UASSERT_NO_THROW(InitSampleMessage(json_str, sample));

    EXPECT_EQ(message.seconds(), 123);
    EXPECT_EQ(message.nanos(), 321);
    CheckMessageEqual(message, sample);
}

TEST(TimestampFromJsonAdditionalTest, InlinedNull) {
    using Message = ::google::protobuf::Timestamp;

    const auto json = formats::json::FromString("null");
    Message message, sample;

    message.set_seconds(100001);

    UASSERT_NO_THROW((message = JsonToMessage<Message>(json)));
    UASSERT_NO_THROW(InitSampleMessage("null", sample));

    EXPECT_EQ(message.seconds(), 0);
    EXPECT_EQ(message.nanos(), 0);
    CheckMessageEqual(message, sample);
}

TEST(TimestampFromJsonAdditionalTest, DynamicMessage) {
    using Message = ::google::protobuf::Timestamp;

    const char* json_str = "\"2016-02-29T00:00:00.123450Z\"";
    const auto json = formats::json::FromString(json_str);
    ::google::protobuf::DynamicMessageFactory factory;

    {
        std::unique_ptr<::google::protobuf::Message> message(factory.GetPrototype(Message::descriptor())->New());

        UASSERT_NO_THROW(JsonToMessage(json, *message));

        const auto reflection = message->GetReflection();
        const auto seconds_desc = message->GetDescriptor()->FindFieldByName("seconds");
        const auto nanos_desc = message->GetDescriptor()->FindFieldByName("nanos");

        EXPECT_EQ(reflection->GetInt64(*message, seconds_desc), 1456704000);
        EXPECT_EQ(reflection->GetInt32(*message, nanos_desc), 123450000);
    }
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
