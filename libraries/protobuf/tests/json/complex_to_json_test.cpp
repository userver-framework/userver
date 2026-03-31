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

TEST(ComplexToJsonSuccessTest, Test) {
    proto_json::messages::ComplexMessage message;
    proto_json::messages::ComplexMessage::Bottom bottom_message;

    for (int i = 1; i <= 2; ++i) {
        auto inter = message.add_inters();
        auto bottoms = inter->mutable_bottoms();
        (*bottoms)["aaa"].set_field1(i * 10);
        (*bottoms)["aaa"].mutable_field2()->set_seconds(i * 1);
        (*bottoms)["aaa"].mutable_field2()->set_nanos(i * 2);
        (*bottoms)["bbb"].set_field1(i * 20);
        (*bottoms)["bbb"].mutable_field2()->set_seconds(i * 3);
        (*bottoms)["bbb"].mutable_field2()->set_nanos(i * 4);
    }

    formats::json::ValueBuilder builder;
    builder = message;

    formats::json::Value json, expected_json, sample_json;
    json = builder.ExtractValue();
    UASSERT_NO_THROW((
        expected_json = formats::json::FromString(
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
        )
    ));

    ProtobufStringType sample_json_str;
    ASSERT_TRUE(::google::protobuf::util::MessageToJsonString(message, &sample_json_str).ok());
    UASSERT_NO_THROW((sample_json = formats::json::FromString(sample_json_str)));

    EXPECT_EQ(json, expected_json);
    EXPECT_EQ(expected_json, sample_json);

    UASSERT_NO_THROW(
        (bottom_message = json["inters"][1]["bottoms"]["aaa"].As<proto_json::messages::ComplexMessage::Bottom>())
    );
    EXPECT_EQ(bottom_message.field1(), std::uint32_t{20});
    EXPECT_EQ(bottom_message.field2().seconds(), 2);
    EXPECT_EQ(bottom_message.field2().nanos(), 4);
}

TEST(ComplexToJsonFailureTest, Test) {
    proto_json::messages::ComplexMessage message;
    proto_json::messages::ComplexMessage::Bottom bottom_message;

    auto inter = message.add_inters();
    auto bottoms = inter->mutable_bottoms();
    (*bottoms)["aaa"].set_field1(10);
    (*bottoms)["aaa"].mutable_field2()->set_seconds(1);
    (*bottoms)["aaa"].mutable_field2()->set_nanos(-1);

    formats::json::ValueBuilder builder;
    EXPECT_PRINT_ERROR((builder = message), PrintErrorCode::kInvalidValue, "inters[0].bottoms['aaa'].field2");
}
*/

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
