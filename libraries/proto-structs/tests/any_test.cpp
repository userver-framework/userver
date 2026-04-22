#include <gtest/gtest.h>

#include <chrono>

#include <google/protobuf/util/time_util.h>

#include <userver/proto-structs/any.hpp>
#include <userver/proto-structs/io/std/chrono/duration_conv.hpp>
#include <userver/utest/assert_macros.hpp>

#include "messages.pb.h"
#include "struct_simple.hpp"

USERVER_NAMESPACE_BEGIN

namespace proto_structs::tests {

TEST(AnyTest, Ctor) {
    Any any_default;
    google::protobuf::Any pbuf_any;

    messages::Simple original;
    original.set_f1(1001);

    ASSERT_TRUE(pbuf_any.PackFrom(original));
    EXPECT_TRUE(any_default.GetProtobufAny().type_url().empty());
    EXPECT_TRUE(std::move(any_default).GetProtobufAny().value().empty());

    Any any = Any{pbuf_any};
    messages::Simple msg;

    ASSERT_TRUE(any.GetProtobufAny().UnpackTo(&msg));
    EXPECT_EQ(msg.f1(), original.f1());

    msg.clear_f1();

    Any any_copy(any);

    ASSERT_TRUE(std::move(any_copy).GetProtobufAny().UnpackTo(&msg));
    EXPECT_EQ(msg.f1(), original.f1());

    any_copy = Any{};
    any_copy = any;

    ASSERT_TRUE(any.GetProtobufAny().UnpackTo(&msg));
    EXPECT_EQ(msg.f1(), original.f1());

    Any any_move(std::move(any_copy));

    ASSERT_TRUE(any.GetProtobufAny().UnpackTo(&msg));
    EXPECT_EQ(msg.f1(), original.f1());

    any_copy = Any{};
    any_copy = std::move(any_move);

    ASSERT_TRUE(any.GetProtobufAny().UnpackTo(&msg));
    EXPECT_EQ(msg.f1(), original.f1());
}

TEST(AnyTest, Pack) {
    structs::Simple obj{.f1 = 1001};
    Any any(obj);

    EXPECT_TRUE(any.Is<structs::Simple>());
    EXPECT_TRUE(any.Is<messages::Simple>());

    messages::Simple msg;

    ASSERT_TRUE(any.GetProtobufAny().UnpackTo(&msg));
    EXPECT_EQ(obj.f1, msg.f1());

    any = {};
    obj.f1 = 1002;
    msg.clear_f1();

    EXPECT_FALSE(any.Is<structs::Simple>());
    EXPECT_FALSE(any.Is<messages::Simple>());
    UASSERT_NO_THROW(any = obj);
    EXPECT_TRUE(any.Is<structs::Simple>());
    EXPECT_TRUE(any.Is<messages::Simple>());
    ASSERT_TRUE(any.GetProtobufAny().UnpackTo(&msg));
    EXPECT_EQ(obj.f1, msg.f1());

    any = {};
    obj.f1 = 1003;
    msg.clear_f1();

    EXPECT_FALSE(any.Is<structs::Simple>());
    EXPECT_FALSE(any.Is<messages::Simple>());
    UASSERT_NO_THROW(any.Pack(structs::Simple{obj}));
    EXPECT_TRUE(any.Is<structs::Simple>());
    EXPECT_TRUE(any.Is<messages::Simple>());
    ASSERT_TRUE(any.GetProtobufAny().UnpackTo(&msg));
    EXPECT_EQ(obj.f1, msg.f1());

    any = {};
    obj.f1 = 1004;
    msg.set_f1(obj.f1);

    EXPECT_FALSE(any.Is<structs::Simple>());
    EXPECT_FALSE(any.Is<messages::Simple>());
    UASSERT_NO_THROW(any.Pack(msg));
    EXPECT_TRUE(any.Is<structs::Simple>());
    EXPECT_TRUE(any.Is<messages::Simple>());

    msg.clear_f1();

    ASSERT_TRUE(any.GetProtobufAny().UnpackTo(&msg));
    EXPECT_EQ(obj.f1, msg.f1());

    UASSERT_NO_THROW(any.Pack(std::chrono::nanoseconds(-123'456'789'987'654'321LL)));
    EXPECT_TRUE(any.Is<std::chrono::milliseconds>());  // any duration suits
    EXPECT_TRUE(any.Is<::google::protobuf::Duration>());

    ::google::protobuf::Duration duration;

    ASSERT_TRUE(any.GetProtobufAny().UnpackTo(&duration));
    EXPECT_EQ(::google::protobuf::util::TimeUtil::DurationToNanoseconds(duration), -123'456'789'987'654'321LL);
}

TEST(AnyTest, Unpack) {
    structs::Simple obj{.f1 = 1001};
    Any any;

    UASSERT_NO_THROW(any.Pack(obj));
    ASSERT_TRUE(any.Is<structs::Simple>());
    EXPECT_EQ(any.Unpack<structs::Simple>().f1, obj.f1);
    ASSERT_TRUE(any.Is<messages::Simple>());
    EXPECT_EQ(any.Unpack<messages::Simple>().f1(), obj.f1);

    UEXPECT_THROW_MSG(static_cast<void>(any.Unpack<structs::Empty>()), AnyUnpackError, "'messages.Empty'");

    any = {};
    messages::Simple msg;
    msg.set_f1(1002);

    EXPECT_FALSE(any.Is<structs::Simple>());
    EXPECT_FALSE(any.Is<messages::Simple>());
    UASSERT_NO_THROW(any.Pack(msg));
    ASSERT_TRUE(any.Is<structs::Simple>());
    EXPECT_EQ(any.Unpack<structs::Simple>().f1, msg.f1());
    ASSERT_TRUE(any.Is<messages::Simple>());
    EXPECT_EQ(any.Unpack<messages::Simple>().f1(), msg.f1());

    auto duration = ::google::protobuf::util::TimeUtil::NanosecondsToDuration(123'456'789'987'654'321LL);

    UASSERT_NO_THROW(any.Pack(std::chrono::nanoseconds(123'456'789'987'654'321LL)));
    ASSERT_TRUE(any.Is<std::chrono::seconds>());  // any duration suits
    EXPECT_EQ(any.Unpack<std::chrono::nanoseconds>(), std::chrono::nanoseconds(123'456'789'987'654'321LL));
    ASSERT_TRUE(any.Is<::google::protobuf::Duration>());
    EXPECT_EQ(any.Unpack<::google::protobuf::Duration>(), duration);
}

}  // namespace proto_structs::tests

USERVER_NAMESPACE_END
