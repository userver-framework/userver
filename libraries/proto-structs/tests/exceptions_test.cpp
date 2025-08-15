#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <userver/proto-structs/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::tests {

TEST(ExceptionsTest, What) {
    {
        proto_structs::ConversionError error("SomeMessage", "SomeField", "SomeReason");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("Message 'SomeMessage'"));
        EXPECT_THAT(error.what(), ::testing::HasSubstr("field 'SomeField'"));
        EXPECT_THAT(error.what(), ::testing::HasSubstr("SomeReason"));
    }

    {
        proto_structs::OneofAccessError error(42);
        EXPECT_THAT(error.what(), ::testing::HasSubstr("index = 42"));
    }

    {
        proto_structs::AnyPackError error("SomeMessage");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("SomeMessage"));
    }

    {
        proto_structs::AnyUnpackError error("SomeMessage");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("SomeMessage"));
    }
}

}  // namespace proto_structs::tests

USERVER_NAMESPACE_END
