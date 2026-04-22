#include <gtest/gtest.h>

#include <limits>

#include <userver/proto-structs/duration.hpp>
#include <userver/proto-structs/exceptions.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utils/impl/internal_tag.hpp>

using namespace std::literals::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace proto_structs::tests {

TEST(DurationTest, IsValid) {
    using OverflowingDuration = std::chrono::duration<std::chrono::seconds::rep, std::chrono::hours::period>;

    EXPECT_FALSE(Duration::IsValid(Duration::kMinSeconds - 1s, 0ns));
    EXPECT_FALSE(Duration::IsValid(Duration::kMaxSeconds + 1s, 0ns));
    EXPECT_FALSE(Duration::IsValid(0s, 1'000'000'000ns));
    EXPECT_FALSE(Duration::IsValid(0s, -1'000'000'000ns));
    EXPECT_FALSE(Duration::IsValid(1s, -1ns));
    EXPECT_FALSE(Duration::IsValid(-1s, 1ns));

    EXPECT_THROW(Duration(Duration::kMinSeconds - 1s, 0ns), ValueError);
    EXPECT_THROW(Duration(Duration::kMinSeconds - 1s), ValueError);
    EXPECT_THROW(Duration{OverflowingDuration::max()}, ValueError);
    EXPECT_THROW(Duration{OverflowingDuration::min()}, ValueError);
    EXPECT_THROW(
        Duration(utils::impl::InternalTag{}, std::numeric_limits<int64_t>::max(), std::numeric_limits<int32_t>::max()),
        ValueError
    );
    EXPECT_THROW(
        Duration(utils::impl::InternalTag{}, std::numeric_limits<int64_t>::min(), std::numeric_limits<int32_t>::min()),
        ValueError
    );
    UEXPECT_THROW_MSG(Duration(987'654'321'000s, 123'456'789ns), ValueError, "987654321000s.123456789ns");
    UEXPECT_THROW_MSG((Duration{OverflowingDuration::max()}), ValueError, "will overflow");

    EXPECT_TRUE(Duration::IsValid(0s, 0ns));
    EXPECT_TRUE(Duration::IsValid(0s, 1ns));
    EXPECT_TRUE(Duration::IsValid(0s, -1ns));
    EXPECT_TRUE(Duration::IsValid(1s, 1ns));
    EXPECT_TRUE(Duration::IsValid(-1s, -1ns));
    EXPECT_TRUE(Duration::IsValid(Duration::Min().Seconds(), Duration::Min().Nanos()));
    EXPECT_TRUE(Duration::IsValid(Duration::Max().Seconds(), Duration::Max().Nanos()));
}

TEST(DurationTest, MinMax) {
    EXPECT_EQ(Duration::Min().Seconds(), Duration::kMinSeconds);
    EXPECT_EQ(Duration::Min().Nanos(), -999'999'999ns);
    EXPECT_EQ(Duration::Max().Seconds(), Duration::kMaxSeconds);
    EXPECT_EQ(Duration::Max().Nanos(), 999'999'999ns);
}

TEST(DurationTest, Conversions) {
    Duration d;

    EXPECT_EQ(d.Seconds(), 0s);
    EXPECT_EQ(d.Nanos(), 0ns);

    d = Duration(123s, 987'654'321ns);

    EXPECT_EQ(d.Seconds(), 123s);
    EXPECT_EQ(d.Nanos(), 987'654'321ns);
    ASSERT_TRUE(d.FitsInChronoDuration<std::chrono::minutes>());
    EXPECT_EQ(d.ToChronoDuration<std::chrono::minutes>(), 2min);
    EXPECT_EQ(static_cast<std::chrono::minutes>(d), 2min);
    EXPECT_EQ(d.ToNanos(), 123'987'654'321ns);
    EXPECT_EQ(d.ToMicros(), 123'987'654us);
    EXPECT_EQ(d.ToMillis(), 123'987ms);

    d = -123'987'654'321ns;

    EXPECT_EQ(d.Seconds(), -123s);
    EXPECT_EQ(d.Nanos(), -987'654'321ns);
    ASSERT_TRUE(d.FitsInChronoDuration<std::chrono::minutes>());
    EXPECT_EQ(d.ToChronoDuration<std::chrono::minutes>(), -2min);
    EXPECT_EQ(static_cast<std::chrono::minutes>(d), -2min);
    EXPECT_EQ(d.ToNanos(), -123'987'654'321ns);
    EXPECT_EQ(d.ToMicros(), -123'987'654us);
    EXPECT_EQ(d.ToMillis(), -123'987ms);

    d = Duration(utils::impl::InternalTag{}, 10, 987);

    EXPECT_EQ(d.Seconds(), 10s);
    EXPECT_EQ(d.Nanos(), 987ns);
}

TEST(DurationTest, SaturateCast) {
    constexpr auto kMaxSeconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::nanoseconds::max());
    constexpr auto kMinSeconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::nanoseconds::min());
    static_assert(kMaxSeconds < Duration::kMaxSeconds && kMinSeconds > Duration::kMinSeconds);

    Duration d;

    UASSERT_NO_THROW(d = std::chrono::nanoseconds::max());
    EXPECT_TRUE(d.FitsInChronoDuration<std::chrono::nanoseconds>());
    EXPECT_EQ(d.ToChronoDuration<std::chrono::nanoseconds>(), std::chrono::nanoseconds::max());
    EXPECT_EQ(static_cast<std::chrono::nanoseconds>(d), std::chrono::nanoseconds::max());
    EXPECT_EQ(d.ToNanos(), std::chrono::nanoseconds::max());

    UASSERT_NO_THROW(d = std::chrono::nanoseconds::min());
    EXPECT_TRUE(d.FitsInChronoDuration<std::chrono::nanoseconds>());
    EXPECT_EQ(d.ToChronoDuration<std::chrono::nanoseconds>(), std::chrono::nanoseconds::min());
    EXPECT_EQ(static_cast<std::chrono::nanoseconds>(d), std::chrono::nanoseconds::min());
    EXPECT_EQ(d.ToNanos(), std::chrono::nanoseconds::min());

    UASSERT_NO_THROW(d = Duration(kMaxSeconds + 1s, 0ns));
    EXPECT_FALSE(d.FitsInChronoDuration<std::chrono::nanoseconds>());
    EXPECT_EQ(d.ToChronoDuration<std::chrono::nanoseconds>(), std::chrono::nanoseconds::max());
    EXPECT_EQ(static_cast<std::chrono::nanoseconds>(d), std::chrono::nanoseconds::max());
    EXPECT_EQ(d.ToNanos(), std::chrono::nanoseconds::max());

    UASSERT_NO_THROW(d = Duration(kMinSeconds - 1s, 0ns));
    EXPECT_FALSE(d.FitsInChronoDuration<std::chrono::nanoseconds>());
    EXPECT_EQ(d.ToChronoDuration<std::chrono::nanoseconds>(), std::chrono::nanoseconds::min());
    EXPECT_EQ(static_cast<std::chrono::nanoseconds>(d), std::chrono::nanoseconds::min());
    EXPECT_EQ(d.ToNanos(), std::chrono::nanoseconds::min());

    UASSERT_NO_THROW(d = Duration(kMaxSeconds + 1s, 123'456'789ns));
    EXPECT_FALSE(d.FitsInChronoDuration<std::chrono::nanoseconds>());
    EXPECT_TRUE(d.FitsInChronoDuration<std::chrono::milliseconds>());
    EXPECT_EQ(d.ToChronoDuration<std::chrono::milliseconds>(), kMaxSeconds + 1s + 123ms);
    EXPECT_EQ(static_cast<std::chrono::milliseconds>(d), kMaxSeconds + 1s + 123ms);
    EXPECT_EQ(d.ToNanos(), std::chrono::nanoseconds::max());
    EXPECT_EQ(d.ToMillis(), kMaxSeconds + 1s + 123ms);

    UASSERT_NO_THROW(d = Duration(kMinSeconds - 1s, -123'456'789ns));
    EXPECT_FALSE(d.FitsInChronoDuration<std::chrono::nanoseconds>());
    EXPECT_TRUE(d.FitsInChronoDuration<std::chrono::milliseconds>());
    EXPECT_EQ(d.ToChronoDuration<std::chrono::milliseconds>(), kMinSeconds - 1s - 123ms);
    EXPECT_EQ(static_cast<std::chrono::milliseconds>(d), kMinSeconds - 1s - 123ms);
    EXPECT_EQ(d.ToNanos(), std::chrono::nanoseconds::min());
    EXPECT_EQ(d.ToMillis(), kMinSeconds - 1s - 123ms);

    {
        using TestDuration = std::chrono::duration<std::int16_t, std::pico>;
        Duration d;

        UASSERT_NO_THROW(d = Duration::Min());
        EXPECT_FALSE(d.FitsInChronoDuration<TestDuration>());
        EXPECT_EQ(d.ToChronoDuration<TestDuration>(), TestDuration::min());
        EXPECT_EQ(static_cast<TestDuration>(d), TestDuration::min());

        UASSERT_NO_THROW(d = Duration::Max());
        EXPECT_FALSE(d.FitsInChronoDuration<TestDuration>());
        EXPECT_EQ(d.ToChronoDuration<TestDuration>(), TestDuration::max());
        EXPECT_EQ(static_cast<TestDuration>(d), TestDuration::max());

        UASSERT_NO_THROW(d = TestDuration::min());
        EXPECT_TRUE(d.FitsInChronoDuration<TestDuration>());
        EXPECT_EQ(d.Seconds(), 0s);
        EXPECT_EQ(d.Nanos(), -32ns);
        EXPECT_EQ(d.ToChronoDuration<TestDuration>().count(), -32'000);
        EXPECT_EQ(static_cast<TestDuration>(d).count(), -32'000);

        UASSERT_NO_THROW(d = TestDuration::max());
        EXPECT_TRUE(d.FitsInChronoDuration<TestDuration>());
        EXPECT_EQ(d.Seconds(), 0s);
        EXPECT_EQ(d.Nanos(), 32ns);
        EXPECT_EQ(d.ToChronoDuration<TestDuration>().count(), 32'000);
        EXPECT_EQ(static_cast<TestDuration>(d).count(), 32'000);
    }

    {
        using TestDuration = std::chrono::duration<std::int16_t, std::chrono::weeks::period>;
        Duration d;

        UASSERT_NO_THROW(d = Duration::Min());
        EXPECT_FALSE(d.FitsInChronoDuration<TestDuration>());
        EXPECT_EQ(d.ToChronoDuration<TestDuration>(), TestDuration::min());
        EXPECT_EQ(static_cast<TestDuration>(d), TestDuration::min());

        UASSERT_NO_THROW(d = Duration::Max());
        EXPECT_FALSE(d.FitsInChronoDuration<TestDuration>());
        EXPECT_EQ(d.ToChronoDuration<TestDuration>(), TestDuration::max());
        EXPECT_EQ(static_cast<TestDuration>(d), TestDuration::max());

        UASSERT_NO_THROW(d = TestDuration::min());
        EXPECT_TRUE(d.FitsInChronoDuration<TestDuration>());
        EXPECT_EQ(d.ToChronoDuration<TestDuration>(), TestDuration::min());
        EXPECT_EQ(static_cast<TestDuration>(d), TestDuration::min());

        UASSERT_NO_THROW(d = TestDuration::max());
        EXPECT_TRUE(d.FitsInChronoDuration<TestDuration>());
        EXPECT_EQ(d.ToChronoDuration<TestDuration>(), TestDuration::max());
        EXPECT_EQ(static_cast<TestDuration>(d), TestDuration::max());
    }
}

}  // namespace proto_structs::tests

USERVER_NAMESPACE_END
