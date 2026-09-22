#include <string>
#include <string_view>

#include <google/protobuf/any.pb.h>
#include <gtest/gtest.h>

#include <protobuf/json/impl/proto_message_visitor.hpp>
#include <protobuf/json/impl/string_writer.hpp>

#include <userver/formats/json/serialize.hpp>
#include <userver/protobuf/json/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "proto_json/messages.pb.h"
#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

namespace {

constexpr std::string_view kSecret = "sensitive-payload";
constexpr std::string_view kSecretBase64 = "c2Vuc2l0aXZlLXBheWxvYWQ=";
constexpr std::size_t kLimit = 1 << 20;

void ExpectNoSecret(std::string_view text) {
    EXPECT_EQ(text.find(kSecret), std::string_view::npos);
    EXPECT_EQ(text.find(kSecretBase64), std::string_view::npos);
}

::google::protobuf::Any MakeUnresolvedAny(
    std::string_view type_url = "type.googleapis.com/testing.unregistered.Message"
) {
    ::google::protobuf::Any any;
    any.set_type_url(std::string{type_url});
    any.set_value(std::string{kSecret});
    return any;
}

}  // namespace

TEST(DebugStringAny, RawAnyOmitsValueWhenRedacted) {
    protobuf::json::impl::StringWriter string_writer{kLimit};
    protobuf::json::impl::ProtoMessageVisitor visitor{string_writer};
    visitor.SetPreserveProtoFieldNames(true);
    visitor.SetExpandAny(false);
    visitor.SetRedactDebugString(true);
    visitor(MakeUnresolvedAny());

    const auto json = string_writer.GetString();
    EXPECT_EQ(json, R"({"type_url":"type.googleapis.com/testing.unregistered.Message"})");
    ExpectNoSecret(json);
}

TEST(DebugStringAny, UnknownTypeOmitsValue) {
    const auto any = MakeUnresolvedAny();
    const auto json = MessageToDebugString(any, kLimit);

    EXPECT_EQ(json, R"({"@type":"type.googleapis.com/testing.unregistered.Message","@error":"unresolved_any_type"})");
    ExpectNoSecret(json);
}

TEST(DebugStringAny, KnownTypeIsExpanded) {
    proto_json::messages::Int32Message payload;
    payload.set_field1(1);
    payload.set_field2(2);
    payload.set_field3(3);

    ::google::protobuf::Any any;
    any.PackFrom(payload);

    const auto json = MessageToDebugString(any, kLimit);
    EXPECT_EQ(
        json,
        R"({"@type":"type.googleapis.com/proto_json.messages.Int32Message","field1":1,"field2":2,"field3":3})"
    );
}

TEST(DebugStringAny, EmptyAny) {
    const ::google::protobuf::Any any;
    EXPECT_EQ(MessageToDebugString(any, kLimit), "{}");
}

TEST(DebugStringAny, NestedUnresolvedAny) {
    ::google::protobuf::Any outer;
    outer.PackFrom(MakeUnresolvedAny());

    const auto json = MessageToDebugString(outer, kLimit);
    EXPECT_EQ(
        json,
        R"({"@type":"type.googleapis.com/google.protobuf.Any","value":{"@type":"type.googleapis.com/testing.unregistered.Message","@error":"unresolved_any_type"}})"
    );
    ExpectNoSecret(json);
}

TEST(DebugStringAny, NeighborsRepeatedAndMap) {
    proto_json::messages::AnyContainer message;
    message.set_keep("keep");
    *message.mutable_any() = MakeUnresolvedAny();

    *message.add_items() = MakeUnresolvedAny("example.com/Unknown");

    proto_json::messages::Int32Message known;
    known.set_field1(1);
    known.set_field2(2);
    known.set_field3(3);
    message.add_items()->PackFrom(known);

    (*message.mutable_entries())["unknown"] = MakeUnresolvedAny("example.com/Unknown");

    const auto json = MessageToDebugString(message, kLimit);
    EXPECT_EQ(
        json,
        R"({"keep":"keep","any":{"@type":"type.googleapis.com/testing.unregistered.Message","@error":"unresolved_any_type"},"items":[{"@type":"example.com/Unknown","@error":"unresolved_any_type"},{"@type":"type.googleapis.com/proto_json.messages.Int32Message","field1":1,"field2":2,"field3":3}],"entries":{"unknown":{"@type":"example.com/Unknown","@error":"unresolved_any_type"}}})"
    );
    ExpectNoSecret(json);
}

TEST(DebugStringAny, EscapedTypeUrl) {
    const std::string type_url = "example.com/\"quoted\"\nMessage";
    const auto json = formats::json::FromString(MessageToDebugString(MakeUnresolvedAny(type_url), kLimit));

    EXPECT_EQ(json["@type"].As<std::string>(), type_url);
    EXPECT_EQ(json["@error"].As<std::string>(), "unresolved_any_type");
    EXPECT_FALSE(json.HasMember("value"));
}

TEST(DebugStringAny, MissingTypeUrlDoesNotLeakValue) {
    proto_json::messages::AnyMessage message;
    message.mutable_field1()->set_value(std::string{kSecret});

    try {
        const auto json = MessageToDebugString(message, kLimit);
        ADD_FAILURE() << "Should throw 'PrintError', got: " << json;
    } catch (const PrintError& error) {
        EXPECT_EQ(error.GetErrorInfo().GetCode(), PrintErrorCode::kInvalidValue);
        EXPECT_EQ(error.GetErrorInfo().GetPath(), "field1");
        ExpectNoSecret(error.what());
    }
}

TEST(DebugStringAny, CorruptKnownPayloadDoesNotLeakValue) {
    proto_json::messages::AnyMessage message;
    message.mutable_field1()->set_type_url("type.googleapis.com/proto_json.messages.Int32Message");
    message.mutable_field1()->set_value(std::string{"\x80"} + std::string{kSecret});

    const auto json = MessageToDebugString(message, kLimit);
    EXPECT_EQ(
        json,
        R"({"field1":{"@type":"type.googleapis.com/proto_json.messages.Int32Message","@error":"invalid_payload"}})"
    );
    ExpectNoSecret(json);
}

TEST(DebugStringAny, StrictJsonStillFailsOnCorruptPayload) {
    proto_json::messages::AnyMessage message;
    message.mutable_field1()->set_type_url("type.googleapis.com/proto_json.messages.Int32Message");
    message.mutable_field1()->set_value(std::string{"\x80"} + std::string{kSecret});

    EXPECT_PRINT_ERROR((void)MessageToJson(message, {}), PrintErrorCode::kInvalidValue, "field1");
}

TEST(DebugStringAny, TruncationDoesNotEmitValue) {
    const auto json = MessageToDebugString(MakeUnresolvedAny(), 16);
    ExpectNoSecret(json);
}

TEST(DebugStringAny, StrictJsonStillFails) {
    proto_json::messages::AnyMessage message;
    *message.mutable_field1() = MakeUnresolvedAny();

    EXPECT_PRINT_ERROR((void)MessageToJson(message, {}), PrintErrorCode::kInvalidValue, "field1");
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
