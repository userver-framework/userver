#include <gtest/gtest.h>

#include <limits>

#include <userver/proto-structs/exceptions.hpp>
#include <userver/proto-structs/time_of_day.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utils/impl/internal_tag.hpp>

using namespace std::literals::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace proto_structs::tests {

TEST(TimeOfDayTest, IsValid) {
    using OverflowingDuration = std::chrono::duration<std::chrono::seconds::rep, std::chrono::hours::period>;
    constexpr auto kMaxInt = std::numeric_limits<std::int32_t>::max();
    constexpr auto kMinInt = std::numeric_limits<std::int32_t>::min();

    EXPECT_FALSE(TimeOfDay::IsValid(-1h, 0min, 0s, 0ns));
    EXPECT_FALSE(TimeOfDay::IsValid(25h, 0min, 0s, 0ns));
    EXPECT_FALSE(TimeOfDay::IsValid(24h, 0min, 0s, 1ns));
    EXPECT_FALSE(TimeOfDay::IsValid(10h, -1min, 0s, 0ns));
    EXPECT_FALSE(TimeOfDay::IsValid(10h, 100min, 0s, 0ns));
    EXPECT_FALSE(TimeOfDay::IsValid(10h, 30min, -1s, 0ns));
    EXPECT_FALSE(TimeOfDay::IsValid(10h, 30min, 100s, 0ns));
    EXPECT_FALSE(TimeOfDay::IsValid(10h, 30min, 45s, -1ns));
    EXPECT_FALSE(TimeOfDay::IsValid(10h, 30min, 45s, std::chrono::duration<int64_t, std::pico>{-1}));
    EXPECT_FALSE(TimeOfDay::IsValid(10h, 30min, 45s, std::chrono::duration<int64_t, std::pico>{1'000'000'000'000LL}));

    EXPECT_THROW([[maybe_unused]] auto val = TimeOfDay(-1h, 0min, 0s, 0ns), ValueError);
    EXPECT_THROW([[maybe_unused]] auto val = TimeOfDay(std::chrono::hh_mm_ss{25h + 30min + 123ns}), ValueError);
    EXPECT_THROW(
        [[maybe_unused]] auto val = TimeOfDay(std::chrono::hh_mm_ss<std::chrono::minutes>{24h + 1min}),
        ValueError
    );
    EXPECT_THROW(
        [[maybe_unused]] auto val = TimeOfDay(std::chrono::duration_cast<std::chrono::milliseconds>(48h)),
        ValueError
    );
    EXPECT_THROW([[maybe_unused]] auto val = TimeOfDay(OverflowingDuration::max()), ValueError);
    EXPECT_THROW(
        [[maybe_unused]] auto val = TimeOfDay(utils::impl::InternalTag{}, kMaxInt, kMaxInt, kMaxInt, kMaxInt),
        ValueError
    );
    EXPECT_THROW(
        [[maybe_unused]] auto val = TimeOfDay(utils::impl::InternalTag{}, kMinInt, kMinInt, kMinInt, kMinInt),
        ValueError
    );
    UEXPECT_THROW_MSG(TimeOfDay(25h, 30min, 0s, 123ns), ValueError, "25:30:0.123ns");
    UEXPECT_THROW_MSG(TimeOfDay(std::chrono::duration<int64_t, std::pico>{-1}), ValueError, "negative");

    EXPECT_TRUE(TimeOfDay::IsValid(0h, 0min, 0s, 0ns));
    EXPECT_TRUE(TimeOfDay::IsValid(23h, 59min, 59s, 999'999'999ns));
    EXPECT_TRUE(TimeOfDay::IsValid(23h, 59min, 60s, 999'999'999ns));
    EXPECT_TRUE(TimeOfDay::IsValid(23h, 59min, 59s, std::chrono::duration<int64_t, std::pico>{999'999'999'999LL}));
    EXPECT_TRUE(TimeOfDay::IsValid(23h, 59min, 60s, std::chrono::duration<int64_t, std::pico>{999'999'999'999LL}));
    EXPECT_TRUE(TimeOfDay::IsValid(24h, 0min, 0s, 0ns));
}

TEST(TimeOfDayTest, Conversions) {
    TimeOfDay t;

    EXPECT_EQ(t.Hours(), 0h);
    EXPECT_EQ(t.Minutes(), 0min);
    EXPECT_EQ(t.Seconds(), 0s);
    EXPECT_EQ(t.Nanos(), 0ns);

    t = TimeOfDay{12h, 30min, 45s, 123'456'789ns};

    EXPECT_EQ(t.Hours(), 12h);
    EXPECT_EQ(t.Minutes(), 30min);
    EXPECT_EQ(t.Seconds(), 45s);
    EXPECT_EQ(t.Nanos(), 123'456'789ns);
    EXPECT_EQ(t.ToChronoDuration(), 12h + 30min + 45s + 123'456'789ns);
    EXPECT_EQ(t.ToChronoDuration<std::chrono::microseconds>(), 12h + 30min + 45s + 123'456us);
    EXPECT_EQ(static_cast<std::chrono::milliseconds>(t), 12h + 30min + 45s + 123ms);
    EXPECT_EQ(t.ToChronoTimeOfDay().hours(), 12h);
    EXPECT_EQ(t.ToChronoTimeOfDay().minutes(), 30min);
    EXPECT_EQ(t.ToChronoTimeOfDay().seconds(), 45s);
    EXPECT_EQ(t.ToChronoTimeOfDay().subseconds(), 123'456'789ns);
    EXPECT_EQ(t.ToChronoTimeOfDay<std::chrono::milliseconds>().hours(), 12h);
    EXPECT_EQ(t.ToChronoTimeOfDay<std::chrono::milliseconds>().minutes(), 30min);
    EXPECT_EQ(t.ToChronoTimeOfDay<std::chrono::milliseconds>().seconds(), 45s);
    EXPECT_EQ(t.ToChronoTimeOfDay<std::chrono::milliseconds>().subseconds(), 123ms);
    EXPECT_EQ(static_cast<std::chrono::hh_mm_ss<std::chrono::microseconds>>(t).hours(), 12h);
    EXPECT_EQ(static_cast<std::chrono::hh_mm_ss<std::chrono::microseconds>>(t).minutes(), 30min);
    EXPECT_EQ(static_cast<std::chrono::hh_mm_ss<std::chrono::microseconds>>(t).seconds(), 45s);
    EXPECT_EQ(static_cast<std::chrono::hh_mm_ss<std::chrono::microseconds>>(t).subseconds(), 123'456us);
    EXPECT_EQ(t.ToUserverDuration().Seconds().count(), (12 * 60 * 60) + (30 * 60) + 45);
    EXPECT_EQ(t.ToUserverDuration().Nanos(), 123'456'789ns);
    EXPECT_EQ(static_cast<Duration>(t).Seconds().count(), (12 * 60 * 60) + (30 * 60) + 45);
    EXPECT_EQ(static_cast<Duration>(t).Nanos(), 123'456'789ns);
    EXPECT_EQ(t.ToUserverTimeOfDay().Hours(), 12h);
    EXPECT_EQ(t.ToUserverTimeOfDay().Minutes(), 30min);
    EXPECT_EQ(t.ToUserverTimeOfDay().Seconds(), 45s);
    EXPECT_EQ(t.ToUserverTimeOfDay().Subseconds(), 123'456'789ns);
    EXPECT_EQ(t.ToUserverTimeOfDay<std::chrono::milliseconds>().Hours(), 12h);
    EXPECT_EQ(t.ToUserverTimeOfDay<std::chrono::milliseconds>().Minutes(), 30min);
    EXPECT_EQ(t.ToUserverTimeOfDay<std::chrono::milliseconds>().Seconds(), 45s);
    EXPECT_EQ(t.ToUserverTimeOfDay<std::chrono::milliseconds>().Subseconds(), 123ms);
    EXPECT_EQ(static_cast<utils::datetime::TimeOfDay<std::chrono::microseconds>>(t).Hours(), 12h);
    EXPECT_EQ(static_cast<utils::datetime::TimeOfDay<std::chrono::microseconds>>(t).Minutes(), 30min);
    EXPECT_EQ(static_cast<utils::datetime::TimeOfDay<std::chrono::microseconds>>(t).Seconds(), 45s);
    EXPECT_EQ(static_cast<utils::datetime::TimeOfDay<std::chrono::microseconds>>(t).Subseconds(), 123'456us);

    t = std::chrono::hh_mm_ss{1h + 15min + 10s + 999ms};

    EXPECT_EQ(t.Hours(), 1h);
    EXPECT_EQ(t.Minutes(), 15min);
    EXPECT_EQ(t.Seconds(), 10s);
    EXPECT_EQ(t.Nanos(), 999'000'000ns);

    t = utils::datetime::TimeOfDay<std::chrono::microseconds>{10h + 11min + 12s + 123456789ns};

    EXPECT_EQ(t.Hours(), 10h);
    EXPECT_EQ(t.Minutes(), 11min);
    EXPECT_EQ(t.Seconds(), 12s);
    EXPECT_EQ(t.Nanos(), 123'456'000ns);

    t = TimeOfDay{1h + 2min + 3s};

    EXPECT_EQ(t.Hours(), 1h);
    EXPECT_EQ(t.Minutes(), 2min);
    EXPECT_EQ(t.Seconds(), 3s);
    EXPECT_EQ(t.Nanos(), 0ns);

    auto point = std::chrono::time_point_cast<std::chrono::system_clock::duration>(std::chrono::sys_days{2025y / 6 / 3}
    );
    point += 22h + 0min + 13s + 555ms;
    t = TimeOfDay{point};

    EXPECT_EQ(t.Hours(), 22h);
    EXPECT_EQ(t.Minutes(), 0min);
    EXPECT_EQ(t.Seconds(), 13s);
    EXPECT_EQ(t.Nanos(), 555'000'000ns);

    t = TimeOfDay{0h, 0min, 60s, 0ns};

    EXPECT_EQ(t.Seconds(), 60s);
    EXPECT_EQ(t.ToChronoTimeOfDay().minutes(), 1min);
    EXPECT_EQ(t.ToChronoTimeOfDay().seconds(), 0s);

    t = TimeOfDay{24h, 0min, 0s, 0ns};

    EXPECT_EQ(t.Hours(), 24h);
    EXPECT_EQ(t.ToUserverTimeOfDay().Hours(), 0h);
    EXPECT_EQ(t.ToUserverTimeOfDay().Minutes(), 0min);
    EXPECT_EQ(t.ToUserverTimeOfDay().Seconds(), 0s);
    EXPECT_EQ(t.ToUserverTimeOfDay().Subseconds(), 0ns);

    t = TimeOfDay{utils::impl::InternalTag{}, 12, 59, 59, 123'456};

    EXPECT_EQ(t.Hours(), 12h);
    EXPECT_EQ(t.Minutes(), 59min);
    EXPECT_EQ(t.Seconds(), 59s);
    EXPECT_EQ(t.Nanos(), 123'456ns);
}

}  // namespace proto_structs::tests

USERVER_NAMESPACE_END
