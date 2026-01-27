#include <gtest/gtest.h>

#include <memory>
#include <ostream>
#include <string>

#include <fmt/format.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/util/json_util.h>

#include <userver/formats/json/serialize.hpp>
#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

struct ValueFromJsonSuccessTestParam {
    std::string input = {};
    ValueMessageData expected_message = {};
    ParseOptions options = {};
};

struct ListValueFromJsonFailureTestParam {
    std::string input = {};
    ParseErrorCode expected_errc = {};
    std::string expected_path = {};
    ParseOptions options = {};

    // We want to skip some native checks because userver implementation has stricter requirements.
    bool skip_native_check = false;
};

struct StructFromJsonFailureTestParam {
    std::string input = {};
    ParseErrorCode expected_errc = {};
    std::string expected_path = {};
    ParseOptions options = {};
};

void PrintTo(const ValueFromJsonSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

void PrintTo(const ListValueFromJsonFailureTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

void PrintTo(const StructFromJsonFailureTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

class ValueFromJsonSuccessTest : public ::testing::TestWithParam<ValueFromJsonSuccessTestParam> {};
class ListValueFromJsonFailureTest : public ::testing::TestWithParam<ListValueFromJsonFailureTestParam> {};
class StructFromJsonFailureTest : public ::testing::TestWithParam<StructFromJsonFailureTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    ValueFromJsonSuccessTest,
    ::
        testing::
            Values(
                ValueFromJsonSuccessTestParam{R"({})", ValueMessageData{}},
                ValueFromJsonSuccessTestParam{R"({"field1":null})", ValueMessageData{ProtoValue{kProtoNullValue}}},
                ValueFromJsonSuccessTestParam{
                    R"({"field1":{}})",
                    ValueMessageData{ProtoValue{std::map<std::string, std::string>{}}}
                },
                ValueFromJsonSuccessTestParam{R"({"field1":[]})", ValueMessageData{ProtoValue{std::vector<double>{}}}},
                ValueFromJsonSuccessTestParam{R"({"field1":1.5})", ValueMessageData{ProtoValue{1.5}}},
                ValueFromJsonSuccessTestParam{R"({"field1":"hello"})", ValueMessageData{ProtoValue{"hello"}}},
                ValueFromJsonSuccessTestParam{R"({"field1":true})", ValueMessageData{ProtoValue{true}}},
                ValueFromJsonSuccessTestParam{
                    R"({"field1":[1.5, 1.5]})",
                    ValueMessageData{ProtoValue{std::vector<double>{1.5, 1.5}}}
                },
                ValueFromJsonSuccessTestParam{
                    R"({"field1":{"aaa":"hello","bbb":"world"}})",
                    ValueMessageData{ProtoValue{std::map<std::string, std::string>{{"aaa", "hello"}, {"bbb", "world"}}}}
                },
                ValueFromJsonSuccessTestParam{
                    R"({"field1":[[]]})",
                    ValueMessageData{std::vector<ProtoValue>{std::vector<double>{}}}
                },
                ValueFromJsonSuccessTestParam{
                    R"({"field1":[[1.5],[0,1.5]]})",
                    ValueMessageData{std::vector<ProtoValue>{std::vector<double>{1.5}, std::vector<double>{0, 1.5}}}
                },
                ValueFromJsonSuccessTestParam{
                    R"({"field1":[null,1.5,"hello",true,[1.5,1.5],{"aaa":"hello","bbb":"world"}]})",
                    ValueMessageData{std::vector<ProtoValue>{
                        kProtoNullValue,
                        1.5,
                        "hello",
                        true,
                        std::vector<double>{1.5, 1.5},
                        std::map<std::string, std::string>{{"aaa", "hello"}, {"bbb", "world"}}
                    }}
                },
                ValueFromJsonSuccessTestParam{
                    R"({"field1":{"aaa":{}}})",
                    ValueMessageData{std::map<std::string, ProtoValue>{{"aaa", std::map<std::string, std::string>{}}}}
                },
                ValueFromJsonSuccessTestParam{
                    R"({"field1":{"aaa":null,"bbb":1.5,"ccc":"hello","ddd":true,"eee":[1.5,1.5],"":{"":"hello","bbb":"world"}}})",
                    ValueMessageData{std::map<std::string, ProtoValue>{
                        {"aaa", kProtoNullValue},
                        {"bbb", 1.5},
                        {"ccc", "hello"},
                        {"ddd", true},
                        {"eee", std::vector<double>{1.5, 1.5}},
                        {"", std::map<std::string, std::string>{{"", "hello"}, {"bbb", "world"}}}
                    }}
                }
            )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    ListValueFromJsonFailureTest,
    ::testing::Values(
        ListValueFromJsonFailureTestParam{R"({})", ParseErrorCode::kInvalidType, "/", {}, true},
        ListValueFromJsonFailureTestParam{R"(true)", ParseErrorCode::kInvalidType, "/"},
        ListValueFromJsonFailureTestParam{R"(10)", ParseErrorCode::kInvalidType, "/"},
        ListValueFromJsonFailureTestParam{R"("")", ParseErrorCode::kInvalidType, "/"}
    )
);

INSTANTIATE_TEST_SUITE_P(
    ,
    StructFromJsonFailureTest,
    ::testing::Values(
        StructFromJsonFailureTestParam{R"([])", ParseErrorCode::kInvalidType, "/"},
        StructFromJsonFailureTestParam{R"(true)", ParseErrorCode::kInvalidType, "/"},
        StructFromJsonFailureTestParam{R"(10)", ParseErrorCode::kInvalidType, "/"},
        StructFromJsonFailureTestParam{R"("")", ParseErrorCode::kInvalidType, "/"}
    )
);

TEST_P(ValueFromJsonSuccessTest, Test) {
    const auto& param = GetParam();

    proto_json::messages::ValueMessage message, expected_message, sample_message;
    formats::json::Value input = PrepareJsonTestData(param.input);
    expected_message = PrepareTestData(param.expected_message);

    message.mutable_field1()->set_number_value(100001);

    UASSERT_NO_THROW((message = JsonToMessage<proto_json::messages::ValueMessage>(input, param.options)));
    UASSERT_NO_THROW(InitSampleMessage(param.input, sample_message, param.options));

    CheckMessageEqual(message, sample_message);
    CheckMessageEqual(message, expected_message);
}

TEST_P(ListValueFromJsonFailureTest, Test) {
    using Message = ::google::protobuf::ListValue;
    const auto& param = GetParam();

    Message sample;
    formats::json::Value input = PrepareJsonTestData(param.input);

    EXPECT_PARSE_ERROR((void)JsonToMessage<Message>(input, param.options), param.expected_errc, param.expected_path);

    if (!param.skip_native_check) {
        EXPECT_FALSE(::google::protobuf::util::JsonStringToMessage(param.input, &sample, {}).ok());
    }
}

TEST_P(StructFromJsonFailureTest, Test) {
    using Message = ::google::protobuf::Struct;
    const auto& param = GetParam();

    Message sample;
    formats::json::Value input = PrepareJsonTestData(param.input);

    EXPECT_PARSE_ERROR((void)JsonToMessage<Message>(input, param.options), param.expected_errc, param.expected_path);
    EXPECT_FALSE(::google::protobuf::util::JsonStringToMessage(param.input, &sample, {}).ok());
}

TEST(ValueFromJsonAdditionalTest, InlinedNonNullValue) {
    using Message = ::google::protobuf::Value;

    {
        const char* json_str = "1.5";
        const auto json = formats::json::FromString(json_str);
        Message message, sample;

        message.set_number_value(100001);

        UASSERT_NO_THROW((message = JsonToMessage<Message>(json)));
        UASSERT_NO_THROW(InitSampleMessage(json_str, sample));
        EXPECT_EQ(message.number_value(), 1.5);
        CheckMessageEqual(message, sample);
    }

    {
        const char* json_str = R"("hello")";
        const auto json = formats::json::FromString(json_str);
        Message message, sample;

        message.set_number_value(100001);

        UASSERT_NO_THROW((message = JsonToMessage<Message>(json)));
        UASSERT_NO_THROW(InitSampleMessage(json_str, sample));
        EXPECT_EQ(message.string_value(), "hello");
        CheckMessageEqual(message, sample);
    }

    {
        const char* json_str = "true";
        const auto json = formats::json::FromString(json_str);
        Message message, sample;

        message.set_number_value(100001);

        UASSERT_NO_THROW((message = JsonToMessage<Message>(json)));
        UASSERT_NO_THROW(InitSampleMessage(json_str, sample));
        EXPECT_TRUE(message.has_bool_value());
        EXPECT_TRUE(message.bool_value());
        CheckMessageEqual(message, sample);
    }

    {
        const char* json_str = "[]";
        const auto json = formats::json::FromString(json_str);
        Message message, sample;

        message.set_number_value(100001);

        UASSERT_NO_THROW((message = JsonToMessage<Message>(json)));
        UASSERT_NO_THROW(InitSampleMessage(json_str, sample));
        EXPECT_TRUE(message.has_list_value());
        EXPECT_TRUE(message.list_value().values().empty());
        CheckMessageEqual(message, sample);
    }

    {
        const char* json_str = "[true, false]";
        const auto json = formats::json::FromString(json_str);
        Message message, sample;

        message.set_number_value(100001);

        UASSERT_NO_THROW((message = JsonToMessage<Message>(json)));
        UASSERT_NO_THROW(InitSampleMessage(json_str, sample));
        EXPECT_TRUE(message.has_list_value());
        ASSERT_EQ(message.list_value().values().size(), 2);
        EXPECT_TRUE(message.list_value().values()[0].has_bool_value());
        EXPECT_TRUE(message.list_value().values()[0].bool_value());
        EXPECT_TRUE(message.list_value().values()[1].has_bool_value());
        EXPECT_FALSE(message.list_value().values()[1].bool_value());
        CheckMessageEqual(message, sample);
    }

    {
        const char* json_str = "{}";
        const auto json = formats::json::FromString(json_str);
        Message message, sample;

        message.set_number_value(100001);

        UASSERT_NO_THROW((message = JsonToMessage<Message>(json)));
        UASSERT_NO_THROW(InitSampleMessage(json_str, sample));
        EXPECT_TRUE(message.has_struct_value());
        EXPECT_EQ(message.struct_value().fields().size(), std::size_t{0});
        CheckMessageEqual(message, sample);
    }

    {
        const char* json_str = R"({"aaa":true,"bbb":false})";
        const auto json = formats::json::FromString(json_str);
        Message message, sample;

        message.set_number_value(100001);

        UASSERT_NO_THROW((message = JsonToMessage<Message>(json)));
        UASSERT_NO_THROW(InitSampleMessage(json_str, sample));
        EXPECT_TRUE(message.has_struct_value());
        EXPECT_EQ(message.struct_value().fields().size(), std::size_t{2});
        ASSERT_TRUE(message.struct_value().fields().contains("aaa"));
        ASSERT_TRUE(message.struct_value().fields().contains("bbb"));
        EXPECT_TRUE(message.struct_value().fields().at("aaa").has_bool_value());
        EXPECT_TRUE(message.struct_value().fields().at("aaa").bool_value());
        EXPECT_TRUE(message.struct_value().fields().at("bbb").has_bool_value());
        EXPECT_FALSE(message.struct_value().fields().at("bbb").bool_value());
        CheckMessageEqual(message, sample);
    }
}

TEST(ValueFromJsonAdditionalTest, InlinedNonNullListValue) {
    using Message = ::google::protobuf::ListValue;

    {
        const char* json_str = "1.5";
        const auto json = formats::json::FromString(json_str);
        Message sample;

        EXPECT_PARSE_ERROR((void)JsonToMessage<Message>(json), ParseErrorCode::kInvalidType, "/");
        UEXPECT_THROW(InitSampleMessage(json_str, sample), SampleError);
    }

    {
        const char* json_str = "[]";
        const auto json = formats::json::FromString(json_str);
        Message message, sample;

        UASSERT_NO_THROW((message = JsonToMessage<Message>(json)));
        UASSERT_NO_THROW(InitSampleMessage(json_str, sample));
        EXPECT_TRUE(message.values().empty());
        CheckMessageEqual(message, sample);
    }

    {
        const char* json_str = R"([null, 1.5, "hello", true])";
        const auto json = formats::json::FromString(json_str);
        Message message, sample;

        UASSERT_NO_THROW((message = JsonToMessage<Message>(json)));
        UASSERT_NO_THROW(InitSampleMessage(json_str, sample));
        ASSERT_EQ(message.values().size(), 4);
        EXPECT_TRUE(message.values()[0].has_null_value());
        EXPECT_TRUE(message.values()[1].has_number_value());
        EXPECT_EQ(message.values()[1].number_value(), 1.5);
        EXPECT_TRUE(message.values()[2].has_string_value());
        EXPECT_EQ(message.values()[2].string_value(), "hello");
        EXPECT_TRUE(message.values()[3].has_bool_value());
        EXPECT_TRUE(message.values()[3].bool_value());
        CheckMessageEqual(message, sample);
    }
}

TEST(ValueFromJsonAdditionalTest, InlinedNonNullStruct) {
    using Message = ::google::protobuf::Struct;

    {
        const char* json_str = "1.5";
        const auto json = formats::json::FromString(json_str);
        Message sample;

        EXPECT_PARSE_ERROR((void)JsonToMessage<Message>(json), ParseErrorCode::kInvalidType, "/");
        UEXPECT_THROW(InitSampleMessage(json_str, sample), SampleError);
    }

    {
        const char* json_str = "{}";
        const auto json = formats::json::FromString(json_str);
        Message message, sample;

        UASSERT_NO_THROW((message = JsonToMessage<Message>(json)));
        UASSERT_NO_THROW(InitSampleMessage(json_str, sample));
        EXPECT_EQ(message.fields().size(), std::size_t{0});
        CheckMessageEqual(message, sample);
    }

    {
        const char* json_str = R"({"aaa":1.5,"":false})";
        const auto json = formats::json::FromString(json_str);
        Message message, sample;

        UASSERT_NO_THROW((message = JsonToMessage<Message>(json)));
        UASSERT_NO_THROW(InitSampleMessage(json_str, sample));
        ASSERT_EQ(message.fields().size(), std::size_t{2});
        ASSERT_TRUE(message.fields().contains("aaa"));
        EXPECT_TRUE(message.fields().at("aaa").has_number_value());
        EXPECT_EQ(message.fields().at("aaa").number_value(), 1.5);
        ASSERT_TRUE(message.fields().contains(""));
        EXPECT_TRUE(message.fields().at("").has_bool_value());
        EXPECT_FALSE(message.fields().at("").bool_value());
        CheckMessageEqual(message, sample);
    }
}

TEST(ValueFromJsonAdditionalTest, InlinedNullValue) {
    using Message = ::google::protobuf::Value;

    const char* json_str = "null";
    const auto json = formats::json::FromString(json_str);
    Message message, sample;

    message.set_number_value(100001);

    UASSERT_NO_THROW((message = JsonToMessage<Message>(json)));
    UASSERT_NO_THROW(InitSampleMessage(json_str, sample));
    EXPECT_TRUE(message.has_null_value());
    CheckMessageEqual(message, sample);
}

TEST(ValueFromJsonAdditionalTest, InlinedNullListValue) {
    using Message = ::google::protobuf::ListValue;

    const char* json_str = "null";
    const auto json = formats::json::FromString(json_str);
    Message message, sample;

    message.add_values()->set_number_value(100001);

    UASSERT_NO_THROW((message = JsonToMessage<Message>(json)));
    UASSERT_NO_THROW(InitSampleMessage(json_str, sample));
    EXPECT_TRUE(message.values().empty());
    CheckMessageEqual(message, sample);
}

TEST(ValueFromJsonAdditionalTest, InlinedNullStruct) {
    using Message = ::google::protobuf::Struct;

    const char* json_str = "null";
    const auto json = formats::json::FromString(json_str);
    Message message, sample;

    (*message.mutable_fields())["aaa"].set_number_value(100001);

    UASSERT_NO_THROW((message = JsonToMessage<Message>(json)));
    UASSERT_NO_THROW(InitSampleMessage(json_str, sample));
    EXPECT_TRUE(message.fields().empty());
    CheckMessageEqual(message, sample);
}

TEST(ValueFromJsonAdditionalTest, DynamicMessage) {
    using Message = ::google::protobuf::Value;

    {
        const char* json_str = "null";
        const auto json = formats::json::FromString(json_str);
        ::google::protobuf::DynamicMessageFactory factory;

        {
            std::unique_ptr<::google::protobuf::Message> message(factory.GetPrototype(Message::descriptor())->New());

            UASSERT_NO_THROW(JsonToMessage(json, *message));

            const auto reflection = message->GetReflection();
            const auto field_desc = message->GetDescriptor()->FindFieldByName("null_value");

            ASSERT_TRUE(reflection->HasField(*message, field_desc));
        }
    }

    {
        const char* json_str = "1.5";
        const auto json = formats::json::FromString(json_str);
        ::google::protobuf::DynamicMessageFactory factory;

        {
            std::unique_ptr<::google::protobuf::Message> message(factory.GetPrototype(Message::descriptor())->New());

            UASSERT_NO_THROW(JsonToMessage(json, *message));

            const auto reflection = message->GetReflection();
            const auto field_desc = message->GetDescriptor()->FindFieldByName("number_value");

            ASSERT_TRUE(reflection->HasField(*message, field_desc));
            EXPECT_EQ(reflection->GetDouble(*message, field_desc), 1.5);
        }
    }

    {
        const char* json_str = R"("hello")";
        const auto json = formats::json::FromString(json_str);
        ::google::protobuf::DynamicMessageFactory factory;

        {
            std::unique_ptr<::google::protobuf::Message> message(factory.GetPrototype(Message::descriptor())->New());

            UASSERT_NO_THROW(JsonToMessage(json, *message));

            const auto reflection = message->GetReflection();
            const auto field_desc = message->GetDescriptor()->FindFieldByName("string_value");

            ASSERT_TRUE(reflection->HasField(*message, field_desc));
            EXPECT_EQ(reflection->GetString(*message, field_desc), "hello");
        }
    }

    {
        const char* json_str = "true";
        const auto json = formats::json::FromString(json_str);
        ::google::protobuf::DynamicMessageFactory factory;

        {
            std::unique_ptr<::google::protobuf::Message> message(factory.GetPrototype(Message::descriptor())->New());

            UASSERT_NO_THROW(JsonToMessage(json, *message));

            const auto reflection = message->GetReflection();
            const auto field_desc = message->GetDescriptor()->FindFieldByName("bool_value");

            ASSERT_TRUE(reflection->HasField(*message, field_desc));
            EXPECT_TRUE(reflection->GetBool(*message, field_desc));
        }
    }

    {
        const char* json_str = "[1.5]";
        const auto json = formats::json::FromString(json_str);
        ::google::protobuf::DynamicMessageFactory factory;

        {
            std::unique_ptr<::google::protobuf::Message> message(factory.GetPrototype(Message::descriptor())->New());

            UASSERT_NO_THROW(JsonToMessage(json, *message));

            const auto reflection = message->GetReflection();
            const auto field_desc = message->GetDescriptor()->FindFieldByName("list_value");

            ASSERT_TRUE(reflection->HasField(*message, field_desc));

            const auto& list = reflection->GetMessage(*message, field_desc);
            const auto list_reflection = list.GetReflection();
            const auto values_desc = list.GetDescriptor()->FindFieldByName("values");

            ASSERT_EQ(list_reflection->FieldSize(list, values_desc), 1);

            const auto& item = list_reflection->GetRepeatedMessage(list, values_desc, 0);
            const auto item_reflection = item.GetReflection();
            const auto number_value_desc = item.GetDescriptor()->FindFieldByName("number_value");

            EXPECT_EQ(item_reflection->GetDouble(item, number_value_desc), 1.5);
        }
    }

    {
        const char* json_str = R"({"aaa":"hello"})";
        const auto json = formats::json::FromString(json_str);
        ::google::protobuf::DynamicMessageFactory factory;

        {
            std::unique_ptr<::google::protobuf::Message> message(factory.GetPrototype(Message::descriptor())->New());

            UASSERT_NO_THROW(JsonToMessage(json, *message));

            const auto reflection = message->GetReflection();
            const auto field_desc = message->GetDescriptor()->FindFieldByName("struct_value");

            ASSERT_TRUE(reflection->HasField(*message, field_desc));

            const auto& obj = reflection->GetMessage(*message, field_desc);
            const auto obj_reflection = obj.GetReflection();
            const auto fields_desc = obj.GetDescriptor()->FindFieldByName("fields");

            ASSERT_EQ(obj_reflection->FieldSize(obj, fields_desc), 1);

            const auto& item = obj_reflection->GetRepeatedMessage(obj, fields_desc, 0);
            const auto item_reflection = item.GetReflection();
            const auto map_key_desc = item.GetDescriptor()->map_key();
            const auto map_value_desc = item.GetDescriptor()->map_value();
            const auto& map_value = item_reflection->GetMessage(item, map_value_desc);
            const auto map_value_reflection = map_value.GetReflection();
            const auto string_value_desc = map_value.GetDescriptor()->FindFieldByName("string_value");

            EXPECT_EQ(item_reflection->GetString(item, map_key_desc), "aaa");
            EXPECT_EQ(map_value_reflection->GetString(map_value, string_value_desc), "hello");
        }
    }
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
