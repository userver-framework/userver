#include <userver/utest/utest.hpp>

#include <userver/ugrpc/proto_json.hpp>

#include <tests/messages.pb.h>

USERVER_NAMESPACE_BEGIN

namespace {

sample::ugrpc::GreetingResponse MakeFilledMessage() {
    sample::ugrpc::GreetingResponse filled_message;
    filled_message.set_name("userver");
    return filled_message;
}

const google::protobuf::util::JsonPrintOptions kEmptyOptions{};

}  // namespace

UTEST(ProtoJson, MessageToJsonStringDefault) {
    const auto json = ugrpc::ToJsonString(MakeFilledMessage());
    ASSERT_EQ(json, R"({"name":"userver","greeting":""})");
}

UTEST(ProtoJson, MessageToJsonStringEmptyOptions) {
    const auto json = ugrpc::ToJsonString(MakeFilledMessage(), kEmptyOptions);
    ASSERT_EQ(json, R"({"name":"userver"})");
}

UTEST(ProtoJson, MessageToJsonDefault) {
    const auto json = ugrpc::MessageToJson(MakeFilledMessage());
    ASSERT_TRUE(json.IsObject());
    EXPECT_EQ(json["greeting"], formats::json::ValueBuilder("").ExtractValue());
    EXPECT_EQ(json["name"], formats::json::ValueBuilder("userver").ExtractValue());
}

UTEST(ProtoJson, MessageToJsonEmptyOptions) {
    const auto json = ugrpc::MessageToJson(MakeFilledMessage(), kEmptyOptions);
    ASSERT_TRUE(json.IsObject());
    EXPECT_FALSE(json.HasMember("greeting"));
    EXPECT_EQ(json["name"], formats::json::ValueBuilder("userver").ExtractValue());
}

USERVER_NAMESPACE_END
