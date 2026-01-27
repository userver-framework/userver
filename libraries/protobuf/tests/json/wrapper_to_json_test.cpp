#include <gtest/gtest.h>

#include <memory>
#include <string>

#include <fmt/format.h>
#include <google/protobuf/dynamic_message.h>

#include <userver/crypto/base64.hpp>
#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utils/from_string.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

template <typename T>
void SetWrappedValue(::google::protobuf::Message& msg, T value) {
    const auto reflection = msg.GetReflection();
    const auto value_desc = msg.GetDescriptor()->FindFieldByName("value");

    if (!value_desc) {
        throw std::runtime_error("Field 'value' is not found in wrapper message");
    }

    if constexpr (std::is_same_v<T, double>) {
        return reflection->SetDouble(&msg, value_desc, value);
    } else if constexpr (std::is_same_v<T, float>) {
        return reflection->SetFloat(&msg, value_desc, value);
    } else if constexpr (std::is_same_v<T, std::int64_t>) {
        return reflection->SetInt64(&msg, value_desc, value);
    } else if constexpr (std::is_same_v<T, std::uint64_t>) {
        return reflection->SetUInt64(&msg, value_desc, value);
    } else if constexpr (std::is_same_v<T, std::int32_t>) {
        return reflection->SetInt32(&msg, value_desc, value);
    } else if constexpr (std::is_same_v<T, std::uint32_t>) {
        return reflection->SetUInt32(&msg, value_desc, value);
    } else if constexpr (std::is_same_v<T, bool>) {
        return reflection->SetBool(&msg, value_desc, value);
    } else if constexpr (std::is_same_v<T, std::string_view>) {
        return reflection->SetString(&msg, value_desc, {value.begin(), value.end()});
    } else {
        static_assert(sizeof(T) && false, "unexpected type");
    }
}

template <typename TParam>
auto GetJsonValue(const formats::json::Value& json) {
    using Type = WrappedType<TParam>;

    if constexpr (std::is_same_v<Type, double>) {
        return json.As<double>();
    } else if constexpr (std::is_same_v<Type, float>) {
        return json.As<float>();
    } else if constexpr (std::is_same_v<Type, std::int64_t> || std::is_same_v<Type, std::uint64_t>) {
        return utils::FromString<Type>(json.As<std::string>());
    } else if constexpr (std::is_same_v<Type, std::int32_t>) {
        return json.As<std::int32_t>();
    } else if constexpr (std::is_same_v<Type, std::uint32_t>) {
        return json.As<std::uint32_t>();
    } else if constexpr (std::is_same_v<Type, bool>) {
        return json.As<bool>();
    } else if constexpr (std::is_same_v<Type, std::string>) {
        if constexpr (!std::is_same_v<TParam, BytesValue>) {
            return json.As<std::string>();
        } else {
            return crypto::base64::Base64Decode(json.As<std::string>());
        }
    } else {
        static_assert(sizeof(TParam) && false, "unexpected type");
    }
}

struct WrapperToJsonSuccessTestParam {
    WrapperMessageData input = {};
    std::string expected_json = {};
    PrintOptions options = {};
};

class WrapperToJsonSuccessTest : public ::testing::TestWithParam<WrapperToJsonSuccessTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    WrapperToJsonSuccessTest,
    ::testing::Values(
        WrapperToJsonSuccessTestParam{WrapperMessageData{}, R"({})"},
        WrapperToJsonSuccessTestParam{
            WrapperMessageData{0, 0, 0, 0, 0, 0, false, "", ""},
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
            })"
        },
        WrapperToJsonSuccessTestParam{
            // using 1.5 for doubles because it is represented without precision loss in IEEE 754
            WrapperMessageData{1.5, 1.5, -123, 123, -321, 321, true, "hello", "world"},
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
            })"
        },
        WrapperToJsonSuccessTestParam{
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
            },
            R"({
              "field3":"-123",
              "field5":-321,
              "field7":true,
              "field9":"d29ybGQ="
            })"
        }
    )
);

TEST_P(WrapperToJsonSuccessTest, Test) {
    const auto& param = GetParam();

    auto input = PrepareTestData(param.input);
    formats::json::Value json, expected_json, sample_json;

    UASSERT_NO_THROW((json = MessageToJson(input, param.options)));
    UASSERT_NO_THROW((expected_json = PrepareJsonTestData(param.expected_json)));
    UASSERT_NO_THROW((sample_json = CreateSampleJson(input, param.options)));

    EXPECT_EQ(json, expected_json);
    EXPECT_EQ(expected_json, sample_json);
}

template <typename T>
class WrapperToJsonAdditionalTest : public ::testing::Test {
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

TYPED_TEST_SUITE(WrapperToJsonAdditionalTest, WrapperTypes);

TYPED_TEST(WrapperToJsonAdditionalTest, InlinedNonNull) {
    using Param = typename TestFixture::Param;
    using Message = typename Param::Message;

    formats::json::Value json, sample;
    Message message;
    SetWrappedValue(message, Param::kValue);

    UASSERT_NO_THROW((json = MessageToJson(message, {})));
    UASSERT_NO_THROW((sample = CreateSampleJson(message)));
    EXPECT_EQ(json, sample);
    EXPECT_EQ(GetJsonValue<Param>(json), Param::kValue);
}

TYPED_TEST(WrapperToJsonAdditionalTest, InlinedNull) {
    using Param = typename TestFixture::Param;
    using Message = typename Param::Message;

    formats::json::Value json, sample;
    Message message;

    UASSERT_NO_THROW((json = MessageToJson(message, {})));
    UASSERT_NO_THROW((sample = CreateSampleJson(message)));
    EXPECT_EQ(json, sample);
}

TYPED_TEST(WrapperToJsonAdditionalTest, DynamicMessage) {
    using Param = typename TestFixture::Param;
    using Message = typename Param::Message;

    ::google::protobuf::DynamicMessageFactory factory;

    {
        std::unique_ptr<::google::protobuf::Message> message(factory.GetPrototype(Message::descriptor())->New());
        SetWrappedValue(*message, Param::kValue);

        formats::json::Value json;

        UASSERT_NO_THROW((json = MessageToJson(*message, {})));
        EXPECT_EQ(GetJsonValue<Param>(json), Param::kValue);
    }
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
