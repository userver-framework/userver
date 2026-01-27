#include <gtest/gtest.h>

#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>

#include <fmt/format.h>
#include <google/protobuf/dynamic_message.h>

#include <userver/formats/json/serialize.hpp>
#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

template <typename T>
T GetWrappedValue(const ::google::protobuf::Message& msg) {
    const auto reflection = msg.GetReflection();
    const auto value_desc = msg.GetDescriptor()->FindFieldByName("value");

    if (!value_desc) {
        throw std::runtime_error("Field 'value' is not found in wrapper message");
    }

    if constexpr (std::is_same_v<T, double>) {
        return reflection->GetDouble(msg, value_desc);
    } else if constexpr (std::is_same_v<T, float>) {
        return reflection->GetFloat(msg, value_desc);
    } else if constexpr (std::is_same_v<T, std::int64_t>) {
        return reflection->GetInt64(msg, value_desc);
    } else if constexpr (std::is_same_v<T, std::uint64_t>) {
        return reflection->GetUInt64(msg, value_desc);
    } else if constexpr (std::is_same_v<T, std::int32_t>) {
        return reflection->GetInt32(msg, value_desc);
    } else if constexpr (std::is_same_v<T, std::uint32_t>) {
        return reflection->GetUInt32(msg, value_desc);
    } else if constexpr (std::is_same_v<T, bool>) {
        return reflection->GetBool(msg, value_desc);
    } else if constexpr (std::is_same_v<T, std::string>) {
        return reflection->GetString(msg, value_desc);
    } else {
        static_assert(sizeof(T) && false, "unexpected type");
    }
}

struct WrapperFromJsonSuccessTestParam {
    std::string input = {};
    WrapperMessageData expected_message = {};
    ParseOptions options = {};
};

struct WrapperFromJsonFailureTestParam {
    std::string input = {};
    ParseErrorCode expected_errc = {};
    std::string expected_path = {};
    ParseOptions options = {};

    // Protobuf ProtoJSON legacy syntax supports out-of-spec object value for wrappers, which we
    // want to prohibit (because we do not want our clients to use syntax that may break in the
    // newer protobuf versions). This variable is used disable some checks that will fail for
    // legacy syntax.
    bool skip_native_check = false;
};

void PrintTo(const WrapperFromJsonFailureTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

class WrapperFromJsonSuccessTest : public ::testing::TestWithParam<WrapperFromJsonSuccessTestParam> {};
class WrapperFromJsonFailureTest : public ::testing::TestWithParam<WrapperFromJsonFailureTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    WrapperFromJsonSuccessTest,
    ::testing::Values(
        WrapperFromJsonSuccessTestParam{R"({})", WrapperMessageData{}},
        WrapperFromJsonSuccessTestParam{
            R"({
               "field1":null,
               "field2":null,
               "field3":null,
               "field4":null,
               "field5":null,
               "field6":null,
               "field7":null,
               "field8":null,
               "field9":null
             })",
            WrapperMessageData{},
        },
        WrapperFromJsonSuccessTestParam{
            R"({
              "field1":0,
              "field2":0,
              "field3":"0",
              "field4":"0",
              "field5":0,
              "field6":0,
              "field7":false,
              "field8":"",
              "field9":""
            })",
            WrapperMessageData{0, 0, 0, 0, 0, 0, false, "", ""},
        },
        WrapperFromJsonSuccessTestParam{
            // using 1.5 for doubles because it is represented without precision loss in IEEE 754
            R"({
               "field1":1.5,
               "field2":1.5,
               "field3":"-123",
               "field4":"123",
               "field5":-321,
               "field6":321,
               "field7":true,
               "field8":"hello",
               "field9":"d29ybGQ="
             })",
            WrapperMessageData{1.5, 1.5, -123, 123, -321, 321, true, "hello", "world"},
        },
        WrapperFromJsonSuccessTestParam{
            // using 1.5 for doubles because it is represented without precision loss in IEEE 754
            R"({
               "field3":"-123",
               "field5":-321,
               "field7":true,
               "field9":"d29ybGQ="
             })",
            WrapperMessageData{
                std::nullopt,
                std::nullopt,
                -123,
                std::nullopt,
                -321,
                std::nullopt,
                true,
                std::nullopt,
                "world"
            }
        }
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    WrapperFromJsonFailureTest,
    ::testing::Values(
        WrapperFromJsonFailureTestParam{R"({"field1":[]})", ParseErrorCode::kInvalidType, "field1"},
        WrapperFromJsonFailureTestParam{R"({"field2":{}})", ParseErrorCode::kInvalidType, "field2", {}, true},
        WrapperFromJsonFailureTestParam{R"({"field3":true})", ParseErrorCode::kInvalidType, "field3"},
        WrapperFromJsonFailureTestParam{R"({"field4":"hello"})", ParseErrorCode::kInvalidValue, "field4"},
        WrapperFromJsonFailureTestParam{R"({"field5":1.5})", ParseErrorCode::kInvalidValue, "field5"}
    )
);

TEST_P(WrapperFromJsonSuccessTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::WrapperMessage message, expected_message, sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);
    expected_message = PrepareTestData(param.expected_message);

    message.mutable_field1()->set_value(100001);
    message.mutable_field2()->set_value(200002);
    message.mutable_field3()->set_value(300003);
    message.mutable_field4()->set_value(400004);
    message.mutable_field5()->set_value(500005);
    message.mutable_field6()->set_value(600006);
    message.mutable_field7()->set_value(true);
    message.mutable_field8()->set_value("dump1");
    message.mutable_field9()->set_value("dump2");

    UASSERT_NO_THROW((message = JsonToMessage<proto_json::messages::WrapperMessage>(input, param.options)));
    UASSERT_NO_THROW(InitSampleMessage(param.input, sample_message, param.options));

    CheckMessageEqual(message, sample_message);
    CheckMessageEqual(message, expected_message);
}

TEST_P(WrapperFromJsonFailureTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::WrapperMessage sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);

    EXPECT_PARSE_ERROR(
        (void)JsonToMessage<proto_json::messages::WrapperMessage>(input, param.options),
        param.expected_errc,
        param.expected_path
    );

    if (!param.skip_native_check) {
        UEXPECT_THROW(InitSampleMessage(param.input, sample_message, param.options), SampleError);
    }
}

template <typename T>
class WrapperFromJsonAdditionalTest : public ::testing::Test {
public:
    using Param = T;
};

using WrapperTypes = ::testing::Types<
    DoubleValue,
    FloatValue,
    Int64Value,
    UInt64Value,
    Int32Value,
    UInt32Value,
    BoolValue,
    StringValue,
    BytesValue>;

TYPED_TEST_SUITE(WrapperFromJsonAdditionalTest, WrapperTypes);

TYPED_TEST(WrapperFromJsonAdditionalTest, InlinedNonNull) {
    using Param = typename TestFixture::Param;
    using Message = typename Param::Message;

    const auto json = formats::json::FromString(Param::kJson);
    Message message, sample;

    UASSERT_NO_THROW((message = JsonToMessage<Message>(json)));
    UASSERT_NO_THROW(InitSampleMessage(Param::kJson, sample));
    EXPECT_EQ(message.value(), sample.value());
    EXPECT_EQ(message.value(), Param::kValue);
}

TYPED_TEST(WrapperFromJsonAdditionalTest, InlinedNull) {
    using Param = typename TestFixture::Param;
    using Message = typename Param::Message;
    using Value = std::remove_const_t<decltype(Param::kValue)>;

    const auto json = formats::json::FromString("null");
    Message message, sample;

    UASSERT_NO_THROW((message = JsonToMessage<Message>(json)));
    UASSERT_NO_THROW(InitSampleMessage("null", sample));
    EXPECT_EQ(message.value(), sample.value());
    EXPECT_EQ(message.value(), Value{});
}

TYPED_TEST(WrapperFromJsonAdditionalTest, DynamicMessage) {
    using Param = typename TestFixture::Param;
    using Message = typename Param::Message;

    const auto json = formats::json::FromString(Param::kJson);
    ::google::protobuf::DynamicMessageFactory factory;

    {
        std::unique_ptr<::google::protobuf::Message> message(factory.GetPrototype(Message::descriptor())->New());

        UASSERT_NO_THROW(JsonToMessage(json, *message));
        EXPECT_EQ(GetWrappedValue<WrappedType<Param>>(*message), Param::kValue);
    }
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
