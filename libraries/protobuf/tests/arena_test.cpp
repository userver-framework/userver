#include <type_traits>

#include <google/protobuf/struct.pb.h>
#include <gtest/gtest.h>

#include <userver/protobuf/arena.hpp>

USERVER_NAMESPACE_BEGIN

TEST(ProtobufArena, CreateMessage) {
    google::protobuf::Arena arena;
    auto message = protobuf::CreateArenaMessage<google::protobuf::ListValue>(arena);
    static_assert(std::is_same_v<decltype(message), utils::NotNull<google::protobuf::ListValue*>>);
    EXPECT_EQ(message->GetArena(), &arena);
    auto* value = message->add_values();
    EXPECT_EQ(value->GetArena(), &arena);
    value->set_string_value("value");
    EXPECT_EQ(message->values(0).string_value(), "value");
}

USERVER_NAMESPACE_END
