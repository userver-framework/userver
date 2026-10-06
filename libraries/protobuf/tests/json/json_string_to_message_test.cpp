#include <gtest/gtest.h>

#include <string_view>

#include <userver/formats/json/exception.hpp>
#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

TEST(JsonStringToMessageTest, Parses) {
    const auto message = JsonStringToMessage<proto_json::messages::StringMessage>(R"({"field1":"aaa"})");
    EXPECT_EQ(message.field1(), "aaa");
}

TEST(JsonStringToMessageTest, OverwritesExistingMessage) {
    proto_json::messages::StringMessage message;
    message.set_field1("old");
    JsonStringToMessage(R"({})", message);
    EXPECT_EQ(message.field1(), "");
    JsonStringToMessage(R"({"field1":"aaa"})", message);
    EXPECT_EQ(message.field1(), "aaa");
}

TEST(JsonStringToMessageTest, InvalidJson) {
    UEXPECT_THROW(
        (void)JsonStringToMessage<proto_json::messages::StringMessage>(R"({"field1":"aaa",})"),
        formats::json::ParseException
    );
}

TEST(JsonStringToMessageTest, UnknownField) {
    constexpr std::string_view kJsonWithUnknownField = R"({"field1":"aaa","unknown":1})";

    EXPECT_PARSE_ERROR(
        (void)JsonStringToMessage<proto_json::messages::StringMessage>(kJsonWithUnknownField),
        ParseErrorCode::kUnknownField,
        "unknown"
    );

    const auto message = JsonStringToMessage<
        proto_json::messages::StringMessage>(kJsonWithUnknownField, {.ignore_unknown_fields = true});
    EXPECT_EQ(message.field1(), "aaa");
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
