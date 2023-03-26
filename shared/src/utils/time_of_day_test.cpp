#include <userver/utils/time_of_day.hpp>

#include <gtest/gtest.h>

#include <fmt/format.h>

#include <userver/utils/fmt_compat.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime::test {

using Hours = TimeOfDay<std::chrono::hours>;
using Mins = TimeOfDay<std::chrono::minutes>;
using Secs = TimeOfDay<std::chrono::seconds>;
using Milli = TimeOfDay<std::chrono::milliseconds>;
using Micro = TimeOfDay<std::chrono::microseconds>;
using Nano = TimeOfDay<std::chrono::nanoseconds>;

template <typename T>
struct TimeOfDayTest : ::testing::Test {};
using TimeOfDayTypes = ::testing::Types<Hours, Mins, Secs, Milli, Micro, Nano>;

TYPED_TEST_SUITE(TimeOfDayTest, TimeOfDayTypes);

TYPED_TEST(TimeOfDayTest, DefaultConstruct) {
  EXPECT_NO_THROW(TypeParam{});
  TypeParam tod;
  EXPECT_EQ(0, tod.Hours().count());
  EXPECT_EQ(0, tod.Minutes().count());
  EXPECT_EQ(0, tod.Seconds().count());
  EXPECT_EQ(0, tod.Subseconds().count());
  EXPECT_EQ(0, tod.SinceMidnight().count());
}

TYPED_TEST(TimeOfDayTest, DurationConstruct) {
  TypeParam tod{std::chrono::hours{10}};
  EXPECT_EQ(10, tod.Hours().count());
  EXPECT_EQ(0, tod.Minutes().count());
  EXPECT_EQ(0, tod.Seconds().count());
  EXPECT_EQ(0, tod.Subseconds().count());

  tod = TypeParam{std::chrono::hours{10} + std::chrono::minutes{10}};
  EXPECT_EQ(10, tod.Hours().count());
  if constexpr (detail::kHasMinutes<TypeParam>) {
    EXPECT_EQ(10, tod.Minutes().count());
  } else {
    EXPECT_EQ(0, tod.Minutes().count()) << fmt::format("{}", tod);
  }
  EXPECT_EQ(0, tod.Seconds().count());
  EXPECT_EQ(0, tod.Subseconds().count());

  tod = TypeParam{std::chrono::hours{25} + std::chrono::minutes{10} +
                  std::chrono::seconds{30}};
  EXPECT_EQ(1, tod.Hours().count());
  if constexpr (detail::kHasMinutes<TypeParam>) {
    EXPECT_EQ(10, tod.Minutes().count());
  } else {
    EXPECT_EQ(0, tod.Minutes().count());
  }
  if constexpr (detail::kHasSeconds<TypeParam>) {
    EXPECT_EQ(30, tod.Seconds().count());
  } else {
    EXPECT_EQ(0, tod.Seconds().count());
  }
  EXPECT_EQ(0, tod.Subseconds().count());
}

TEST(TimeOfDay, StringConstruct) {
  EXPECT_THROW(Milli{"1"}, std::runtime_error) << "Expect minutes";
  EXPECT_THROW(Milli{"25"}, std::runtime_error) << "Expect hours <= 24";
  EXPECT_THROW(Milli{"1:"}, std::runtime_error)
      << "Expect 2 digits for minutes";
  EXPECT_THROW(Milli{"1:1"}, std::runtime_error)
      << "Expect 2 digits for minutes";
  EXPECT_THROW(Milli{"1:100"}, std::runtime_error)
      << "Expect 2 digits for minutes";
  EXPECT_THROW(Milli{"1:60"}, std::runtime_error) << "Expect minutes < 60";
  EXPECT_THROW(Milli{"1:60:00"}, std::runtime_error) << "Expect minutes < 60";
  EXPECT_THROW(Milli{"1:00:"}, std::runtime_error)
      << "Expect 2 digits for seconds";
  EXPECT_THROW(Milli{"1:00:1"}, std::runtime_error)
      << "Expect 2 digits for seconds";
  EXPECT_THROW(Milli{"1:00:111"}, std::runtime_error)
      << "Expect 2 digits for seconds";
  EXPECT_THROW(Milli{"1:00:60"}, std::runtime_error) << "Expect seconds < 60";
  EXPECT_THROW(Milli{"1:00:60.0"}, std::runtime_error) << "Expect seconds < 60";
  EXPECT_THROW(Milli{"1:00:00:0"}, std::runtime_error)
      << "Expect decimal point";
  EXPECT_THROW(Milli{"1:00:30."}, std::runtime_error)
      << "Expect some digits for subseconds";

  EXPECT_THROW(Milli{"1a"}, std::runtime_error) << "Unexpected chars";
  EXPECT_THROW(Milli{"1:a"}, std::runtime_error) << "Unexpected chars";
  EXPECT_THROW(Milli{"1:15:a"}, std::runtime_error) << "Unexpected chars";
  EXPECT_THROW(Milli{"1:15:2a"}, std::runtime_error) << "Unexpected chars";
  EXPECT_THROW(Milli{"1:15:20.a"}, std::runtime_error) << "Unexpected chars";

  EXPECT_THROW(Milli{"25:00"}, std::runtime_error) << "Out of range";
  EXPECT_THROW(Milli{"24:01"}, std::runtime_error) << "Out of range";
  {
    Milli tod{"1:30"};
    EXPECT_EQ(1, tod.Hours().count());
    EXPECT_EQ(30, tod.Minutes().count());
    EXPECT_EQ(0, tod.Seconds().count());
    EXPECT_EQ(0, tod.Subseconds().count());
  }
  {
    Milli tod{"10:20:30.1"};
    EXPECT_EQ(10, tod.Hours().count());
    EXPECT_EQ(20, tod.Minutes().count());
    EXPECT_EQ(30, tod.Seconds().count());
    EXPECT_EQ(100, tod.Subseconds().count());
  }
  {
    Milli tod{"10:20:30.001"};
    EXPECT_EQ(10, tod.Hours().count());
    EXPECT_EQ(20, tod.Minutes().count());
    EXPECT_EQ(30, tod.Seconds().count());
    EXPECT_EQ(1, tod.Subseconds().count());
  }
  {
    Milli tod{"10:20:30.0012345"};
    EXPECT_EQ(10, tod.Hours().count());
    EXPECT_EQ(20, tod.Minutes().count());
    EXPECT_EQ(30, tod.Seconds().count());
    EXPECT_EQ(1, tod.Subseconds().count());
  }
  {
    Mins tod{"24:00"};
    EXPECT_EQ(0, tod.Hours().count());
    EXPECT_EQ(0, tod.Minutes().count());
    EXPECT_EQ(0, tod.Seconds().count());
    EXPECT_EQ(0, tod.Subseconds().count());
  }
}

TEST(TimeOfDay, DefaultFormat) {
  EXPECT_EQ("01:00", fmt::format("{}", Hours{"01:20"}));
  EXPECT_EQ("01:20", fmt::format("{}", Mins{"01:20"}));
  EXPECT_EQ("01:20:30", fmt::format("{}", Secs{"01:20:30"}));

  EXPECT_EQ("01:20:30", fmt::format("{}", Milli{"01:20:30"}));
  EXPECT_EQ("01:20:30", fmt::format("{}", Micro{"01:20:30"}));
  EXPECT_EQ("01:20:30", fmt::format("{}", Nano{"01:20:30"}));
}

TEST(TimeOfDay, Format) {
  Milli tod{std::chrono::hours{1} + std::chrono::minutes{2} +
            std::chrono::seconds{3}};
  EXPECT_THROW(static_cast<void>(fmt::format(fmt::runtime("{:%}"), tod)),
               fmt::format_error);
  EXPECT_THROW(static_cast<void>(fmt::format(fmt::runtime("{:%o}"), tod)),
               fmt::format_error);
  EXPECT_THROW(static_cast<void>(fmt::format(fmt::runtime("{:%H%}"), tod)),
               fmt::format_error);

  EXPECT_EQ("01:02:03", fmt::format("{}", tod)) << "Default format";
  EXPECT_EQ("01:02:03", fmt::format("{:%H:%M:%S}", tod)) << "HH:MM:SS format";
  EXPECT_EQ("010203", fmt::format("{:%H%M%S}", tod)) << "HHMMSS format";
  EXPECT_EQ("01%02%03", fmt::format("{:%H%%%M%%%S}", tod)) << "HHMMSS format";
  EXPECT_EQ("01:02", fmt::format("{:%H:%M}", tod)) << "HH:MM format";
  EXPECT_EQ("01h 02m 03s", fmt::format("{:%Hh %Mm %Ss}", tod))
      << "HHh MMm SSs format";
}

TEST(TimeOfDay, Arithmetic) {
  EXPECT_EQ(std::chrono::hours{1}, Hours{"02:00"} - Hours{"01:00"});
  EXPECT_EQ(std::chrono::hours{1}, Mins{"02:30"} - Mins{"01:30"});
  EXPECT_EQ(Mins{"05:45"}, Mins{"04:45"} + std::chrono::hours{1});
  EXPECT_EQ(Mins{"03:45"}, Mins{"04:45"} - std::chrono::hours{1});

  EXPECT_EQ(Mins{"06:30"}, Mins{"06:30"} + std::chrono::hours{24});
  EXPECT_EQ(Mins{"06:30"}, Mins{"06:30"} - std::chrono::hours{24});
}

}  // namespace utils::datetime::test

USERVER_NAMESPACE_END
