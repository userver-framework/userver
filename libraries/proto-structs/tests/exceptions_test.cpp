#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fmt/format.h>

#include <userver/proto-structs/exceptions.hpp>

TEST(ExceptionsTest, What) {
    {
        proto_structs::AnyPackError error("SomeMessage");
        EXPECT_THAT(error.what(), testing::HasSubstr("SomeMessage"));
    }

    {
        proto_structs::AnyUnpackError error("SomeMessage");
        EXPECT_THAT(error.what(), testing::HasSubstr("SomeMessage"));
    }
}
