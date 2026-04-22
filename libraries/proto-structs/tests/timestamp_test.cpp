#include <gtest/gtest.h>

#include <limits>

#include <userver/proto-structs/exceptions.hpp>
#include <userver/proto-structs/timestamp.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utils/impl/internal_tag.hpp>

using namespace std::literals::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace proto_structs::tests {

TEST(TimestampTest, IsValid) {
    using OverflowingDuration = std::chrono::duration<std::chrono::seconds::rep, std::chrono::hours::period>;
    constexpr auto kMaxSystemClockDuration = std::chrono::system_clock::time_point::max().time_since_epoch();

    EXPECT_FALSE(Timestamp::IsValid(Timestamp::kMinSeconds - 1s, 0ns));
    EXPECT_FALSE(Timestamp::IsValid(Timestamp::kMaxSeconds + 1s, 0ns));
    EXPECT_FALSE(Timestamp::IsValid(0s, 1'000'000'000ns));
    EXPECT_FALSE(Timestamp::IsValid(0s, -1ns));
    EXPECT_FALSE(Timestamp::IsValid(1s, -1ns));
    EXPECT_FALSE(Timestamp::IsValid(-1s, -1ns));

    EXPECT_THROW(Timestamp(Timestamp::kMinSeconds - 1s, 0ns), ValueError);
    EXPECT_THROW(Timestamp{Duration::Min()}, ValueError);
    EXPECT_THROW(Timestamp{Duration::Max()}, ValueError);
    EXPECT_THROW(Timestamp(Timestamp::kMinSeconds - 1s), ValueError);
    EXPECT_THROW(Timestamp(Timestamp::kMaxSeconds + 1s), ValueError);

    if constexpr (std::chrono::duration_cast<std::chrono::seconds>(kMaxSystemClockDuration) > Timestamp::kMaxSeconds) {
        EXPECT_THROW(Timestamp{std::chrono::system_clock::time_point::max()}, ValueError);
    }

    EXPECT_THROW(
        Timestamp(utils::impl::InternalTag{}, std::numeric_limits<int64_t>::max(), std::numeric_limits<int32_t>::max()),
        ValueError
    );
    EXPECT_THROW(
        Timestamp(utils::impl::InternalTag{}, std::numeric_limits<int64_t>::min(), std::numeric_limits<int32_t>::min()),
        ValueError
    );

    UEXPECT_THROW_MSG(Timestamp(-100'654'321'000s, 123'456'789ns), ValueError, "-100654321000s.123456789ns");
    UEXPECT_THROW_MSG((Timestamp{OverflowingDuration::max()}), ValueError, "will overflow");

    EXPECT_TRUE(Timestamp::IsValid(0s, 0ns));
    EXPECT_TRUE(Timestamp::IsValid(0s, 1ns));
    EXPECT_TRUE(Timestamp::IsValid(1s, 1ns));
    EXPECT_TRUE(Timestamp::IsValid(Timestamp::Min().Seconds(), Timestamp::Min().Nanos()));
    EXPECT_TRUE(Timestamp::IsValid(Timestamp::Max().Seconds(), Timestamp::Max().Nanos()));
}

TEST(TimestampTest, MinMax) {
    EXPECT_EQ(Timestamp::Min().Seconds(), Timestamp::kMinSeconds);
    EXPECT_EQ(Timestamp::Min().Nanos(), 0ns);
    EXPECT_EQ(Timestamp::Min().GetTimeSinceEpoch().Seconds(), Timestamp::kMinSeconds);
    EXPECT_EQ(Timestamp::Min().GetTimeSinceEpoch().Nanos(), 0ns);
    EXPECT_EQ(Timestamp::Max().Seconds(), Timestamp::kMaxSeconds);
    EXPECT_EQ(Timestamp::Max().Nanos(), 999'999'999ns);
    EXPECT_EQ(Timestamp::Max().GetTimeSinceEpoch().Seconds(), Timestamp::kMaxSeconds);
    EXPECT_EQ(Timestamp::Max().GetTimeSinceEpoch().Nanos(), 999'999'999ns);
}

TEST(TimestampTest, Conversions) {
    using SysClockDuration = std::chrono::system_clock::time_point::duration;

    Timestamp t;

    EXPECT_EQ(t.Seconds(), 0s);
    EXPECT_EQ(t.Nanos(), 0ns);
    ASSERT_TRUE(t.FitsInChronoTimePoint());
    EXPECT_EQ(t.ToTimePoint().time_since_epoch(), std::chrono::duration_cast<SysClockDuration>(0ns));
    EXPECT_EQ(t.GetTimeSinceEpoch().Seconds(), 0s);
    EXPECT_EQ(t.GetTimeSinceEpoch().Nanos(), 0ns);

    t = Timestamp(123s, 987'654'321ns);

    EXPECT_EQ(t.Seconds(), 123s);
    EXPECT_EQ(t.Nanos(), 987'654'321ns);
    ASSERT_TRUE(t.FitsInChronoTimePoint());
    EXPECT_EQ(t.ToTimePoint().time_since_epoch(), std::chrono::duration_cast<SysClockDuration>(123s + 987'654'321ns));
    EXPECT_EQ(t.GetTimeSinceEpoch().Seconds(), 123s);
    EXPECT_EQ(t.GetTimeSinceEpoch().Nanos(), 987'654'321ns);

    t = Timestamp{Duration(333s, 123'456'789ns)};

    EXPECT_EQ(t.Seconds(), 333s);
    EXPECT_EQ(t.Nanos(), 123'456'789ns);
    ASSERT_TRUE(t.FitsInChronoTimePoint());
    EXPECT_EQ(t.ToTimePoint().time_since_epoch(), std::chrono::duration_cast<SysClockDuration>(333s + 123'456'789ns));
    EXPECT_EQ(t.GetTimeSinceEpoch().Seconds(), 333s);
    EXPECT_EQ(t.GetTimeSinceEpoch().Nanos(), 123'456'789ns);

    t = Timestamp{Duration(-333s, -123'456'789ns)};

    EXPECT_EQ(t.Seconds(), -334s);
    EXPECT_EQ(t.Nanos(), -123'456'789ns + 1s);
    ASSERT_TRUE(t.FitsInChronoTimePoint());
    EXPECT_EQ(t.ToTimePoint().time_since_epoch(), std::chrono::duration_cast<SysClockDuration>(-333s - 123'456'789ns));
    EXPECT_EQ(t.GetTimeSinceEpoch().Seconds(), -333s);
    EXPECT_EQ(t.GetTimeSinceEpoch().Nanos(), -123'456'789ns);

    t = Timestamp{555'987'654'321ns};

    EXPECT_EQ(t.Seconds(), 555s);
    EXPECT_EQ(t.Nanos(), 987'654'321ns);
    ASSERT_TRUE(t.FitsInChronoTimePoint());
    EXPECT_EQ(t.ToTimePoint().time_since_epoch(), std::chrono::duration_cast<SysClockDuration>(555s + 987'654'321ns));
    EXPECT_EQ(t.GetTimeSinceEpoch().Seconds(), 555s);
    EXPECT_EQ(t.GetTimeSinceEpoch().Nanos(), 987'654'321ns);

    t = Timestamp{-555'987'654'321ns};

    EXPECT_EQ(t.Seconds(), -556s);
    EXPECT_EQ(t.Nanos(), -987'654'321ns + 1s);
    ASSERT_TRUE(t.FitsInChronoTimePoint());
    EXPECT_EQ(t.GetTimeSinceEpoch().Seconds(), -555s);
    EXPECT_EQ(t.GetTimeSinceEpoch().Nanos(), -987'654'321ns);

    std::chrono::system_clock::time_point point{std::chrono::duration_cast<SysClockDuration>(777'111'222'333ns)};
    t = point;

    EXPECT_EQ(t.Seconds(), 777s);
    EXPECT_EQ(t.Nanos(), std::chrono::duration_cast<SysClockDuration>(111'222'333ns));
    ASSERT_TRUE(t.FitsInChronoTimePoint());
    EXPECT_EQ(t.ToTimePoint().time_since_epoch(), std::chrono::duration_cast<SysClockDuration>(777s + 111'222'333ns));
    EXPECT_EQ(t.GetTimeSinceEpoch().Seconds(), 777s);
    EXPECT_EQ(t.GetTimeSinceEpoch().Nanos(), std::chrono::duration_cast<SysClockDuration>(111'222'333ns));

    t = Timestamp{utils::impl::InternalTag{}, 1001, 333'555'777};

    EXPECT_EQ(t.Seconds(), 1001s);
    EXPECT_EQ(t.Nanos(), 333'555'777ns);
    ASSERT_TRUE(t.FitsInChronoTimePoint());
    EXPECT_EQ(t.ToTimePoint().time_since_epoch(), std::chrono::duration_cast<SysClockDuration>(1001s + 333'555'777ns));
    EXPECT_EQ(t.GetTimeSinceEpoch().Seconds(), 1001s);
    EXPECT_EQ(t.GetTimeSinceEpoch().Nanos(), 333'555'777ns);
}

TEST(TimestampTest, SaturateCast) {
    if constexpr (!Timestamp::Min().GetTimeSinceEpoch().FitsInChronoDuration<std::chrono::system_clock::duration>()) {
        EXPECT_EQ(Timestamp::Min().ToTimePoint(), std::chrono::system_clock::time_point::min());
    }

    if constexpr (!Timestamp::Max().GetTimeSinceEpoch().FitsInChronoDuration<std::chrono::system_clock::duration>()) {
        EXPECT_EQ(Timestamp::Max().ToTimePoint(), std::chrono::system_clock::time_point::max());
    }
}

}  // namespace proto_structs::tests

USERVER_NAMESPACE_END
