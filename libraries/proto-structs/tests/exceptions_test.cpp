#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <userver/proto-structs/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::tests {

TEST(ExceptionsTest, What) {
    {
        proto_structs::ReadError error("msg.field.nested_field", "oops");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("msg.field.nested_field"));
        EXPECT_THAT(error.what(), ::testing::HasSubstr("oops"));
    }

    {
        proto_structs::WriteError error("msg.field.nested_field", "oops");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("msg.field.nested_field"));
        EXPECT_THAT(error.what(), ::testing::HasSubstr("oops"));
    }

    {
        proto_structs::OneofAccessError error(42);
        EXPECT_THAT(error.what(), ::testing::HasSubstr("index = 42"));
    }

    {
        proto_structs::AnyPackError error("some message");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("some message"));
    }

    {
        proto_structs::AnyUnpackError error("some message");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("some message"));
    }
}

}  // namespace proto_structs::tests

USERVER_NAMESPACE_END
