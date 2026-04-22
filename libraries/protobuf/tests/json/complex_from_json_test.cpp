#include <gtest/gtest.h>

#include <google/protobuf/util/json_util.h>

#include <userver/formats/json/serialize.hpp>
#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

/* NOTE: Uncomment when linkage is fixed (see comment in the convert.hpp)

using ProtobufStringType =
    decltype(std::declval<::google::protobuf::Reflection>()
                 .GetString(std::declval<const ::google::protobuf::Message&>(), nullptr));

TEST(ComplexFromJsonSuccessTest, Test) {
    const auto json = formats::json::FromString(
        R"({
              "inters": [
                {
                  "bottoms": {
                    "aaa": {"field1":10, "field2":"1.000000002s"},
                    "bbb": {"field1":20, "field2":"3.000000004s"}
                  }
                },
                {
                  "bottoms": {
                    "aaa": {"field1":20, "field2":"2.000000004s"},
                    "bbb": {"field1":40, "field2":"6.000000008s"}
                  }
                }
              ]
          })"
    );

    proto_json::messages::ComplexMessage message;
    proto_json::messages::ComplexMessage::Bottom bottom_message;
    ::google::protobuf::Struct struct_message;
    ::google::protobuf::Value value_message;

    UASSERT_NO_THROW((message = json.As<proto_json::messages::ComplexMessage>()));
    ASSERT_EQ(message.inters().size(), 2);
    EXPECT_EQ(message.inters()[0].bottoms().size(), std::size_t{2});

    ASSERT_TRUE(message.inters()[0].bottoms().contains("aaa"));
    EXPECT_EQ(message.inters()[0].bottoms().at("aaa").field1(), std::uint32_t{10});
    EXPECT_EQ(message.inters()[0].bottoms().at("aaa").field2().seconds(), 1);
    EXPECT_EQ(message.inters()[0].bottoms().at("aaa").field2().nanos(), 2);

    ASSERT_TRUE(message.inters()[0].bottoms().contains("bbb"));
    EXPECT_EQ(message.inters()[0].bottoms().at("bbb").field1(), std::uint32_t{20});
    EXPECT_EQ(message.inters()[0].bottoms().at("bbb").field2().seconds(), 3);
    EXPECT_EQ(message.inters()[0].bottoms().at("bbb").field2().nanos(), 4);

    ASSERT_TRUE(message.inters()[1].bottoms().contains("aaa"));
    EXPECT_EQ(message.inters()[1].bottoms().at("aaa").field1(), std::uint32_t{20});
    EXPECT_EQ(message.inters()[1].bottoms().at("aaa").field2().seconds(), 2);
    EXPECT_EQ(message.inters()[1].bottoms().at("aaa").field2().nanos(), 4);

    ASSERT_TRUE(message.inters()[1].bottoms().contains("bbb"));
    EXPECT_EQ(message.inters()[1].bottoms().at("bbb").field1(), std::uint32_t{40});
    EXPECT_EQ(message.inters()[1].bottoms().at("bbb").field2().seconds(), 6);
    EXPECT_EQ(message.inters()[1].bottoms().at("bbb").field2().nanos(), 8);

    UASSERT_NO_THROW(
        (bottom_message = json["inters"][0]["bottoms"]["bbb"].As<proto_json::messages::ComplexMessage::Bottom>())
    );
    EXPECT_EQ(bottom_message.field1(), std::size_t{20});
    EXPECT_EQ(bottom_message.field2().seconds(), 3);
    EXPECT_EQ(bottom_message.field2().nanos(), 4);

    UASSERT_NO_THROW((struct_message = json["inters"][1]["bottoms"]["bbb"].As<::google::protobuf::Struct>()));
    EXPECT_EQ(struct_message.fields().size(), std::size_t{2});
    ASSERT_TRUE(struct_message.fields().contains("field1"));
    ASSERT_TRUE(struct_message.fields().at("field1").has_number_value());
    EXPECT_EQ(struct_message.fields().at("field1").number_value(), std::uint32_t{40});
    ASSERT_TRUE(struct_message.fields().contains("field2"));
    ASSERT_TRUE(struct_message.fields().at("field2").has_string_value());
    EXPECT_EQ(struct_message.fields().at("field2").string_value(), "6.000000008s");

    UASSERT_NO_THROW((value_message = json["inters"][1]["bottoms"]["aaa"]["field1"].As<::google::protobuf::Value>()));
    ASSERT_TRUE(value_message.has_number_value());
    EXPECT_EQ(value_message.number_value(), std::uint32_t{20});
}

TEST(ComplexFromJsonFailureTest, Test) {
    const auto
        json = formats::json::FromString(R"({"inters":[{"bottoms":{"some.key": {"field1":10, "field2":"oops"}}}]})");

    proto_json::messages::ComplexMessage message;
    proto_json::messages::ComplexMessage::Bottom bottom_message;

    EXPECT_PARSE_ERROR(
        (message = json.As<proto_json::messages::ComplexMessage>()),
        ParseErrorCode::kInvalidValue,
        "inters[0].bottoms.'some.key'.field2"
    );
    EXPECT_PARSE_ERROR(
        (bottom_message = json["inters"][0]["bottoms"]["some.key"].As<proto_json::messages::ComplexMessage::Bottom>()),
        ParseErrorCode::kInvalidValue,
        "field2"
    );
}
*/

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
