#include <gtest/gtest.h>

#include <limits>
#include <memory>
#include <string>

#include <fmt/format.h>
#include <google/protobuf/dynamic_message.h>

#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

struct ValueToJsonSuccessTestParam {
    ValueMessageData input = {};
    std::string expected_json = {};
    PrintOptions options = {};
};

struct ValueToJsonFailureTestParam {
    ValueMessageData input = {};
    PrintErrorCode expected_errc = {};
    std::string expected_path = {};
    PrintOptions options = {};

    // We want to skip some native checks because userver implementation has stricter requirements.
    bool skip_native_check = false;
};

class ValueToJsonSuccessTest : public ::testing::TestWithParam<ValueToJsonSuccessTestParam> {};
class ValueToJsonFailureTest : public ::testing::TestWithParam<ValueToJsonFailureTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    ValueToJsonSuccessTest,
    ::
        testing::
            Values(
                ValueToJsonSuccessTestParam{ValueMessageData{}, R"({})"},
                ValueToJsonSuccessTestParam{ValueMessageData{ProtoValue{kProtoNullValue}}, R"({"field1":null})"},
                ValueToJsonSuccessTestParam{ValueMessageData{ProtoValue{1.5}}, R"({"field1":1.5})"},
                ValueToJsonSuccessTestParam{ValueMessageData{ProtoValue{"hello"}}, R"({"field1":"hello"})"},
                ValueToJsonSuccessTestParam{ValueMessageData{ProtoValue{true}}, R"({"field1":true})"},
                ValueToJsonSuccessTestParam{ValueMessageData{ProtoValue{std::vector<double>{}}}, R"({"field1":[]})"},
                ValueToJsonSuccessTestParam{
                    ValueMessageData{ProtoValue{std::vector<double>{1.5, 1.5}}},
                    R"({"field1":[1.5, 1.5]})"
                },
                ValueToJsonSuccessTestParam{
                    ValueMessageData{ProtoValue{std::map<std::string, std::string>{}}},
                    R"({"field1":{}})"
                },
                ValueToJsonSuccessTestParam{
                    ValueMessageData{ProtoValue{std::map<std::string, std::string>{{"aaa", "hello"}, {"bbb", "world"}}}
                    },
                    R"({"field1":{"aaa":"hello","bbb":"world"}})"
                },
                ValueToJsonSuccessTestParam{
                    ValueMessageData{
                        std::vector<ProtoValue>{},
                    },
                    R"({"field1":[]})"
                },
                ValueToJsonSuccessTestParam{
                    ValueMessageData{
                        std::vector<ProtoValue>{std::vector<double>{}},
                    },
                    R"({"field1":[[]]})"
                },
                ValueToJsonSuccessTestParam{
                    ValueMessageData{
                        std::vector<ProtoValue>{std::vector<double>{1.5}, std::vector<double>{0, 1.5}},
                    },
                    R"({"field1":[[1.5],[0,1.5]]})"
                },
                ValueToJsonSuccessTestParam{
                    ValueMessageData{
                        std::vector<ProtoValue>{
                            kProtoNullValue,
                            1.5,
                            "hello",
                            true,
                            std::vector<double>{1.5, 1.5},
                            std::map<std::string, std::string>{{"aaa", "hello"}, {"bbb", "world"}}
                        },
                    },
                    R"({"field1":[null,1.5,"hello",true,[1.5,1.5],{"aaa":"hello","bbb":"world"}]})"
                },
                ValueToJsonSuccessTestParam{
                    ValueMessageData{
                        std::map<std::string, ProtoValue>{},
                    },
                    R"({"field1":{}})"
                },
                ValueToJsonSuccessTestParam{
                    ValueMessageData{
                        std::map<std::string, ProtoValue>{{"aaa", ProtoValue{std::map<std::string, std::string>{}}}},
                    },
                    R"({"field1":{"aaa":{}}})"
                },
                ValueToJsonSuccessTestParam{
                    ValueMessageData{
                        std::map<std::string, ProtoValue>{
                            {"aaa", kProtoNullValue},
                            {"bbb", 1.5},
                            {"ccc", "hello"},
                            {"ddd", true},
                            {"eee", std::vector<double>{1.5, 1.5}},
                            {"", std::map<std::string, std::string>{{"", "hello"}, {"bbb", "world"}}}
                        },
                    },
                    R"({"field1":{"aaa":null,"bbb":1.5,"ccc":"hello","ddd":true,"eee":[1.5,1.5],"":{"":"hello","bbb":"world"}}})"
                }
            )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    ValueToJsonFailureTest,
    ::testing::Values(
        ValueToJsonFailureTestParam{
            ValueMessageData{ProtoValue{std::monostate{}}},
            PrintErrorCode::kInvalidValue,
            "field1",
            {},
            true  // native implementation silently discards 'Value' without any alternative set which we find bug-prone
        },
        ValueToJsonFailureTestParam{
            ValueMessageData{ProtoValue{std::numeric_limits<double>::quiet_NaN()}},
            PrintErrorCode::kInvalidValue,
            "field1.number_value"
        },
        ValueToJsonFailureTestParam{
            ValueMessageData{ProtoValue{std::numeric_limits<double>::signaling_NaN()}},
            PrintErrorCode::kInvalidValue,
            "field1.number_value"
        },
        ValueToJsonFailureTestParam{
            ValueMessageData{ProtoValue{std::numeric_limits<double>::infinity()}},
            PrintErrorCode::kInvalidValue,
            "field1.number_value"
        },
        ValueToJsonFailureTestParam{
            ValueMessageData{ProtoValue{-std::numeric_limits<double>::infinity()}},
            PrintErrorCode::kInvalidValue,
            "field1.number_value"
        },
        ValueToJsonFailureTestParam{
            ValueMessageData{std::vector<ProtoValue>{ProtoValue{1.5}, ProtoValue{std::monostate{}}, ProtoValue{true}}},
            PrintErrorCode::kInvalidValue,
            "field1.list_value.values[1]",
            {},
            true
        },
        ValueToJsonFailureTestParam{
            ValueMessageData{std::vector<ProtoValue>{
                ProtoValue{1.5},
                ProtoValue{std::vector<double>{1.5, std::numeric_limits<double>::infinity()}},
                ProtoValue{true}
            }},
            PrintErrorCode::kInvalidValue,
            "field1.list_value.values[1].list_value.values[1].number_value"
        },
        ValueToJsonFailureTestParam{
            ValueMessageData{std::map<
                std::string,
                ProtoValue>{{"aaa", ProtoValue{1.5}}, {"bbb", ProtoValue{std::monostate{}}}, {"ccc", ProtoValue{true}}}
            },
            PrintErrorCode::kInvalidValue,
            "field1.struct_value.fields['bbb']",
            {},
            true
        }
    )
);

TEST_P(ValueToJsonSuccessTest, Test) {
    const auto& param = GetParam();

    auto input = PrepareTestData(param.input);
    formats::json::Value json, expected_json, sample_json;

    UASSERT_NO_THROW((json = MessageToJson(input, param.options)));
    UASSERT_NO_THROW((expected_json = PrepareJsonTestData(param.expected_json)));
    UASSERT_NO_THROW((sample_json = CreateSampleJson(input, param.options)));

    EXPECT_EQ(json, expected_json);
    EXPECT_EQ(expected_json, sample_json);
}

TEST_P(ValueToJsonFailureTest, Test) {
    const auto& param = GetParam();
    auto input = PrepareTestData(param.input);

    EXPECT_PRINT_ERROR((void)MessageToJson(input, param.options), param.expected_errc, param.expected_path);

    if (!param.skip_native_check) {
        UEXPECT_THROW((void)CreateSampleJson(input, param.options), SampleError);
    }
}

TEST(ValueToJsonAdditionalTest, InlinedNonNull) {
    {
        ValueMessageData data{kProtoNullValue};
        auto message = PrepareTestData(data);
        formats::json::Value json, sample;

        UASSERT_NO_THROW((json = MessageToJson(message.field1(), {})));
        UASSERT_NO_THROW((sample = CreateSampleJson(message.field1())));
        EXPECT_EQ(json, sample);
        EXPECT_TRUE(json.IsNull());
    }

    {
        ValueMessageData data{1.5};
        auto message = PrepareTestData(data);
        formats::json::Value json, sample;

        UASSERT_NO_THROW((json = MessageToJson(message.field1(), {})));
        UASSERT_NO_THROW((sample = CreateSampleJson(message.field1())));
        EXPECT_EQ(json, sample);
        ASSERT_TRUE(json.IsNumber());
        EXPECT_EQ(json.As<double>(), 1.5);
    }

    {
        ValueMessageData data{"hello"};
        auto message = PrepareTestData(data);
        formats::json::Value json, sample;

        UASSERT_NO_THROW((json = MessageToJson(message.field1(), {})));
        UASSERT_NO_THROW((sample = CreateSampleJson(message.field1())));
        EXPECT_EQ(json, sample);
        EXPECT_TRUE(json.IsString());
        EXPECT_EQ(json.As<std::string>(), "hello");
    }

    {
        ValueMessageData data{true};
        auto message = PrepareTestData(data);
        formats::json::Value json, sample;

        UASSERT_NO_THROW((json = MessageToJson(message.field1(), {})));
        UASSERT_NO_THROW((sample = CreateSampleJson(message.field1())));
        EXPECT_EQ(json, sample);
        EXPECT_TRUE(json.IsBool());
        EXPECT_TRUE(json.As<bool>());
    }

    {
        ValueMessageData data{std::vector<double>{}};
        auto message = PrepareTestData(data);
        formats::json::Value json, sample;

        UASSERT_NO_THROW((json = MessageToJson(message.field1(), {})));
        UASSERT_NO_THROW((sample = CreateSampleJson(message.field1())));
        EXPECT_EQ(json, sample);
        EXPECT_TRUE(json.IsArray());
        ASSERT_EQ(json.GetSize(), std::size_t{0});
    }

    {
        ValueMessageData data{std::vector<double>{1.5, 0}};
        auto message = PrepareTestData(data);
        formats::json::Value json, sample;

        UASSERT_NO_THROW((json = MessageToJson(message.field1(), {})));
        UASSERT_NO_THROW((sample = CreateSampleJson(message.field1())));
        EXPECT_EQ(json, sample);
        EXPECT_TRUE(json.IsArray());
        ASSERT_EQ(json.GetSize(), std::size_t{2});
        EXPECT_EQ(json[0].As<double>(), 1.5);
        EXPECT_EQ(json[1].As<double>(), 0);
    }

    {
        ValueMessageData data{std::map<std::string, std::string>{}};
        auto message = PrepareTestData(data);
        formats::json::Value json, sample;

        UASSERT_NO_THROW((json = MessageToJson(message.field1(), {})));
        UASSERT_NO_THROW((sample = CreateSampleJson(message.field1())));
        EXPECT_EQ(json, sample);
        EXPECT_TRUE(json.IsObject());
        ASSERT_EQ(json.GetSize(), std::size_t{0});
    }

    {
        ValueMessageData data{std::map<std::string, std::string>{{"aaa", "hello"}, {"bbb", "world"}}};
        auto message = PrepareTestData(data);
        formats::json::Value json, sample;

        UASSERT_NO_THROW((json = MessageToJson(message.field1(), {})));
        UASSERT_NO_THROW((sample = CreateSampleJson(message.field1())));
        EXPECT_EQ(json, sample);
        EXPECT_TRUE(json.IsObject());
        ASSERT_EQ(json.GetSize(), std::size_t{2});
        ASSERT_TRUE(json.HasMember("aaa"));
        ASSERT_TRUE(json.HasMember("bbb"));
        EXPECT_EQ(json["aaa"].As<std::string>(), "hello");
        EXPECT_EQ(json["bbb"].As<std::string>(), "world");
    }
}

TEST(ValueToJsonAdditionalTest, InlinedNull) {
    ValueMessageData data{std::monostate{}};
    auto message = PrepareTestData(data);

    EXPECT_PRINT_ERROR((void)MessageToJson(message.field1(), {}), PrintErrorCode::kInvalidValue, "/");
}

TEST(ValueToJsonAdditionalTest, DynamicMessage) {
    using Message = ::google::protobuf::Value;
    ::google::protobuf::DynamicMessageFactory factory;

    {
        std::unique_ptr<::google::protobuf::Message> message(factory.GetPrototype(Message::descriptor())->New());
        const auto reflection = message->GetReflection();
        const auto null_value_desc = message->GetDescriptor()->FindFieldByName("null_value");

        reflection->SetEnumValue(message.get(), null_value_desc, 0);

        formats::json::Value json;

        UASSERT_NO_THROW((json = MessageToJson(*message, {})));
        ASSERT_TRUE(json.IsNull());
    }

    {
        std::unique_ptr<::google::protobuf::Message> message(factory.GetPrototype(Message::descriptor())->New());
        const auto reflection = message->GetReflection();
        const auto number_value_desc = message->GetDescriptor()->FindFieldByName("number_value");

        reflection->SetDouble(message.get(), number_value_desc, 1.5);

        formats::json::Value json;

        UASSERT_NO_THROW((json = MessageToJson(*message, {})));
        ASSERT_TRUE(json.IsNumber());
        EXPECT_EQ(json.As<double>(), 1.5);
    }

    {
        std::unique_ptr<::google::protobuf::Message> message(factory.GetPrototype(Message::descriptor())->New());
        const auto reflection = message->GetReflection();
        const auto number_value_desc = message->GetDescriptor()->FindFieldByName("string_value");

        reflection->SetString(message.get(), number_value_desc, "hello");

        formats::json::Value json;

        UASSERT_NO_THROW((json = MessageToJson(*message, {})));
        ASSERT_TRUE(json.IsString());
        EXPECT_EQ(json.As<std::string>(), "hello");
    }

    {
        std::unique_ptr<::google::protobuf::Message> message(factory.GetPrototype(Message::descriptor())->New());
        const auto reflection = message->GetReflection();
        const auto bool_value_desc = message->GetDescriptor()->FindFieldByName("bool_value");

        reflection->SetBool(message.get(), bool_value_desc, true);

        formats::json::Value json;

        UASSERT_NO_THROW((json = MessageToJson(*message, {})));
        ASSERT_TRUE(json.IsBool());
        EXPECT_EQ(json.As<bool>(), true);
    }

    {
        std::unique_ptr<::google::protobuf::Message> message(factory.GetPrototype(Message::descriptor())->New());
        const auto reflection = message->GetReflection();
        const auto list_value_desc = message->GetDescriptor()->FindFieldByName("list_value");
        const auto list_value_message = reflection->MutableMessage(message.get(), list_value_desc, &factory);
        const auto list_value_reflection = list_value_message->GetReflection();
        const auto values_desc = list_value_message->GetDescriptor()->FindFieldByName("values");
        const auto item_message = list_value_reflection->AddMessage(list_value_message, values_desc, &factory);
        const auto item_reflection = item_message->GetReflection();
        const auto number_value_desc = item_message->GetDescriptor()->FindFieldByName("number_value");

        item_reflection->SetDouble(item_message, number_value_desc, 1.5);

        formats::json::Value json;

        UASSERT_NO_THROW((json = MessageToJson(*message, {})));
        ASSERT_TRUE(json.IsArray());
        ASSERT_EQ(json.GetSize(), std::size_t{1});
        EXPECT_EQ(json[0].As<double>(), 1.5);
    }

    {
        std::unique_ptr<::google::protobuf::Message> message(factory.GetPrototype(Message::descriptor())->New());
        const auto reflection = message->GetReflection();
        const auto struct_value_desc = message->GetDescriptor()->FindFieldByName("struct_value");
        const auto struct_value_message = reflection->MutableMessage(message.get(), struct_value_desc, &factory);
        const auto struct_value_reflection = struct_value_message->GetReflection();
        const auto fields_desc = struct_value_message->GetDescriptor()->FindFieldByName("fields");
        const auto item_message = struct_value_reflection->AddMessage(struct_value_message, fields_desc, &factory);
        const auto item_reflection = item_message->GetReflection();
        const auto map_key_desc = item_message->GetDescriptor()->map_key();
        const auto map_value_desc = item_message->GetDescriptor()->map_value();
        const auto map_value_message = item_reflection->MutableMessage(item_message, map_value_desc, &factory);
        const auto map_value_reflection = map_value_message->GetReflection();
        const auto number_value_desc = map_value_message->GetDescriptor()->FindFieldByName("number_value");

        item_reflection->SetString(item_message, map_key_desc, "aaa");
        map_value_reflection->SetDouble(map_value_message, number_value_desc, 1.5);

        formats::json::Value json;

        UASSERT_NO_THROW((json = MessageToJson(*message, {})));
        ASSERT_TRUE(json.IsObject());
        ASSERT_EQ(json.GetSize(), std::size_t{1});
        ASSERT_TRUE(json.HasMember("aaa"));
        EXPECT_EQ(json["aaa"].As<double>(), 1.5);
    }
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
