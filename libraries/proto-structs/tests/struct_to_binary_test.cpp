#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <userver/proto-structs/convert.hpp>
#include <userver/utest/assert_macros.hpp>

#include "messages.pb.h"
#include "structs.hpp"

USERVER_NAMESPACE_BEGIN

namespace proto_structs::tests {

TEST(StructToBinary, Scalar) {
    {
        messages::Scalar msg;
        structs::Scalar obj{
            .f1 = true,
            .f2 = std::numeric_limits<int32_t>::min(),
            .f3 = std::numeric_limits<uint32_t>::max(),
            .f4 = std::numeric_limits<int64_t>::min(),
            .f5 = std::numeric_limits<uint64_t>::max(),
            .f6 = static_cast<float>(1.5),
            .f7 = -2.5,
            .f8 = "test1",
            .f9 = "test2",
            .f10 = structs::TestEnum::kValue1,
            .f11 = 987
        };

        std::string serialized_from_obj = StructToBinary(obj);
        ASSERT_TRUE(msg.ParseFromString(serialized_from_obj));

        CheckScalarEqual(obj, msg);
    }
}
}  // namespace proto_structs::tests

USERVER_NAMESPACE_END
