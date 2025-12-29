#include <gtest/gtest.h>

#include <userver/protobuf/datetime.hpp>

USERVER_NAMESPACE_BEGIN

namespace protobuf::tests {

TEST(DatetimeTest, IsValidDuration) {
    EXPECT_TRUE(IsValidDuration(kMinDurationSeconds, kMinDurationNanos));
    EXPECT_TRUE(IsValidDuration(kMaxDurationSeconds, kMaxDurationNanos));
    EXPECT_TRUE(IsValidDuration(100, 123'456'789));
    EXPECT_TRUE(IsValidDuration(-100, -123'456'789));
    EXPECT_TRUE(IsValidDuration(1, 0));
    EXPECT_TRUE(IsValidDuration(-1, 0));
    EXPECT_TRUE(IsValidDuration(0, 1));
    EXPECT_TRUE(IsValidDuration(0, -1));
    EXPECT_TRUE(IsValidDuration(0, 0));

    EXPECT_FALSE(IsValidDuration(kMinDurationSeconds - 1, 0));
    EXPECT_FALSE(IsValidDuration(kMaxDurationSeconds + 1, 0));
    EXPECT_FALSE(IsValidDuration(0, kMinDurationNanos - 1));
    EXPECT_FALSE(IsValidDuration(0, kMaxDurationNanos + 1));
    EXPECT_FALSE(IsValidDuration(1, -1));
    EXPECT_FALSE(IsValidDuration(-1, 1));
}

TEST(DatetimeTest, IsValidTimestamp) {
    EXPECT_TRUE(IsValidTimestamp(kMinTimestampSeconds, kMinTimestampNanos));
    EXPECT_TRUE(IsValidTimestamp(kMaxTimestampSeconds, kMaxTimestampNanos));
    EXPECT_TRUE(IsValidTimestamp(100, 123'456'789));
    EXPECT_TRUE(IsValidTimestamp(1, 0));
    EXPECT_TRUE(IsValidTimestamp(-1, 0));
    EXPECT_TRUE(IsValidTimestamp(-1, 1));
    EXPECT_TRUE(IsValidTimestamp(0, 1));
    EXPECT_TRUE(IsValidTimestamp(0, 0));

    EXPECT_FALSE(IsValidTimestamp(kMinTimestampSeconds - 1, 0));
    EXPECT_FALSE(IsValidTimestamp(kMaxTimestampSeconds + 1, 0));
    EXPECT_FALSE(IsValidTimestamp(0, kMinTimestampNanos - 1));
    EXPECT_FALSE(IsValidTimestamp(0, kMaxTimestampNanos + 1));
    EXPECT_FALSE(IsValidTimestamp(1, -1));
    EXPECT_FALSE(IsValidTimestamp(0, -1));
}

}  // namespace protobuf::tests

USERVER_NAMESPACE_END
