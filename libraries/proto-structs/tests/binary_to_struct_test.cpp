#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <userver/proto-structs/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "messages.pb.h"
#include "structs.hpp"

USERVER_NAMESPACE_BEGIN

namespace proto_structs::tests {

TEST(BinaryToStruct, Scalar) {
    {
        messages::Scalar msg;
        msg.set_f1(true);
        msg.set_f2(std::numeric_limits<int32_t>::min());
        msg.set_f3(std::numeric_limits<uint32_t>::max());
        msg.set_f4(std::numeric_limits<int64_t>::min());
        msg.set_f5(std::numeric_limits<uint64_t>::max());
        msg.set_f6(static_cast<float>(1.5));
        msg.set_f7(-2.5);
        msg.set_f8("test1");
        msg.set_f9("test2");
        msg.set_f10(messages::TestEnum::TEST_ENUM_VALUE1);
        msg.set_f11(987);

        protobuf::ProtoStringType serialized_from_msg_tstring;
        ASSERT_TRUE(msg.SerializeToString(&serialized_from_msg_tstring));
        std::string_view serialized_from_msg = serialized_from_msg_tstring;

        {
            structs::Scalar obj;
            UASSERT_NO_THROW(BinaryToStruct(serialized_from_msg, obj));
            CheckScalarEqual(obj, msg);
        }

        {
            auto obj = BinaryToStruct<structs::Scalar>(serialized_from_msg);
            CheckScalarEqual(obj, msg);
        }
    }
}

TEST(BinaryToStruct, ThrowsOnInvalidBinary) {
    structs::Scalar obj;

    // Неверная длина строк
    std::string binary_data;
    // Поле f8 (string, field number 8) → tag = 0x42 (8 << 3 | 2)
    binary_data += '\x42';
    // Указываем длину 100, но даём только 5 байт
    binary_data += '\x64';   // 100 в varint
    binary_data += "hello";  // только 5 байт

    UEXPECT_THROW(BinaryToStruct<structs::Scalar>(binary_data, obj), ReadError);
    UEXPECT_THROW((void)BinaryToStruct<structs::Scalar>(binary_data), ReadError);
}
}  // namespace proto_structs::tests

USERVER_NAMESPACE_END
