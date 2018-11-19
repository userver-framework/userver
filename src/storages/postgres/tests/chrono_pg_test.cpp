#include <gtest/gtest.h>

#include <storages/postgres/io/chrono.hpp>
#include <storages/postgres/io/force_text.hpp>
#include <storages/postgres/test_buffers.hpp>
#include <storages/postgres/tests/util_test.hpp>

namespace pg = storages::postgres;
namespace io = pg::io;

namespace static_test {

static_assert(
    (io::traits::HasFormatter<std::chrono::system_clock::time_point,
                              io::DataFormat::kTextDataFormat>::value == true),
    "");
static_assert((io::traits::HasParser<std::chrono::system_clock::time_point,
                                     io::DataFormat::kTextDataFormat>::value ==
               true),
              "");
static_assert(
    (io::traits::HasFormatter<std::chrono::high_resolution_clock::time_point,
                              io::DataFormat::kTextDataFormat>::value == true),
    "");
static_assert(
    (io::traits::HasParser<std::chrono::high_resolution_clock::time_point,
                           io::DataFormat::kTextDataFormat>::value == true),
    "");

}  // namespace static_test

namespace {

TEST(PostgreIO, Chrono) {
  {
    auto now = std::chrono::system_clock::now();
    pg::test::Buffer buffer;
    EXPECT_NO_THROW(
        io::WriteBuffer<io::DataFormat::kTextDataFormat>(buffer, now));
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::DataFormat::kTextDataFormat);
    std::chrono::system_clock::time_point tgt;
    EXPECT_NO_THROW(io::ReadBuffer<io::DataFormat::kTextDataFormat>(fb, tgt));
    EXPECT_EQ(now, tgt) << "Parse buffer "
                        << std::string{buffer.begin(), buffer.end()};
  }
  {
    auto now = std::chrono::high_resolution_clock::now();
    pg::test::Buffer buffer;
    EXPECT_NO_THROW(
        io::WriteBuffer<io::DataFormat::kTextDataFormat>(buffer, now));
    auto fb =
        pg::test::MakeFieldBuffer(buffer, io::DataFormat::kTextDataFormat);
    std::chrono::high_resolution_clock::time_point tgt;
    EXPECT_NO_THROW(io::ReadBuffer<io::DataFormat::kTextDataFormat>(fb, tgt));
    EXPECT_EQ(now, tgt) << "Parse buffer "
                        << std::string{buffer.begin(), buffer.end()};
  }
}

template <typename Duration>
std::string FormatToTZ(
    const std::chrono::time_point<std::chrono::system_clock, Duration>& tp,
    const cctz::time_zone& tz) {
  static const std::string ts_format_tz = "%Y-%m-%d %H:%M:%E*S%Ez";
  return cctz::format(ts_format_tz, tp, tz);
}

template <typename Duration>
std::string FormatToUtc(
    const std::chrono::time_point<std::chrono::system_clock, Duration>& tp) {
  static const auto utc = cctz::utc_time_zone();
  return FormatToTZ(tp, utc);
}

template <typename Duration>
std::string FormatToLocal(
    const std::chrono::time_point<std::chrono::system_clock, Duration>& tp) {
  return FormatToTZ(tp, cctz::local_time_zone());
}

pg::TimePoint ParseUTC(const ::std::string& value) {
  static const auto utc = cctz::utc_time_zone();
  static const std::string ts_format_tz = "%Y-%m-%d %H:%M:%E*S%Ez";
  pg::TimePoint tp;
  cctz::parse(ts_format_tz, value, utc, &tp);
  return tp;
}

::testing::AssertionResult EqualToMicroseconds(
    const std::chrono::system_clock::time_point& lhs,
    const std::chrono::system_clock::time_point& rhs) {
  auto diff = std::chrono::duration_cast<std::chrono::microseconds>(lhs - rhs);
  if (diff == std::chrono::microseconds{0}) {
    return ::testing::AssertionSuccess();
  } else {
    return ::testing::AssertionFailure()
           << "∆ between " << FormatToLocal(lhs) << " and "
           << FormatToLocal(rhs) << " is " << diff.count() << "µs";
  }
}

// RAII class to set TZ environment variable. Not for use with threads.
class TemporaryTZ {
 public:
  TemporaryTZ(const std::string& tz) : current_tz_{tz}, old_tz_{} {
    auto env = std::getenv("TZ");
    if (env) {
      std::string{env}.swap(old_tz_);
    }
    SetTz(tz);
  }
  ~TemporaryTZ() { SetTz(old_tz_); }

 private:
  void SetTz(const std::string& tz) {
    if (tz.empty()) {
      ::unsetenv("TZ");
    } else {
      ::setenv("TZ", tz.c_str(), 1);
    }
  }
  std::string current_tz_;
  std::string old_tz_;

  friend std::ostream& operator<<(std::ostream& os, const TemporaryTZ& val) {
    os << "Client TZ: "
       << (val.current_tz_.empty() ? "local" : val.current_tz_);
    return os;
  }
};

void CheckInTimezone(pg::detail::ConnectionPtr& conn,
                     const std::string& tz_name = {}) {
  TemporaryTZ tmp_tz{tz_name};
  auto tz = cctz::local_time_zone();
  if (!tz_name.empty()) {
    ASSERT_TRUE(cctz::load_time_zone(tz_name, &tz))
        << "cctz load time zone " << tz_name;
    ASSERT_NO_THROW(conn->SetParameter(
        "TimeZone", tz_name, pg::detail::Connection::ParameterScope::kSession))
        << "Set timezone for Postgres " << tmp_tz;
  }  // Else use local timezone

  std::vector<pg::TimePoint> timepoints{
      pg::ClockType::now(),
      ParseUTC("2018-01-01 10:00:00+00"),  // Some time in winter
      ParseUTC(
          "2018-07-01 10:00:00+00")  // Some time in summer, for DST timezones
  };

  EXPECT_NO_THROW(conn->Execute(
      "create temp table tstest(fmt text, notz timestamp, tz timestamptz)"));
  for (auto src : timepoints) {
    EXPECT_NO_THROW(conn->ExperimentalExecute(
        "insert into tstest(fmt, notz, tz) values ($1, $2, $3)",
        pg::io::DataFormat::kTextDataFormat, "txt", pg::ForceTextFormat(src),
        pg::ForceTextFormat(pg::TimestampTz(src))));
    EXPECT_NO_THROW(conn->ExperimentalExecute(
        "insert into tstest(fmt, notz, tz) values ($1, $2, $3)",
        pg::io::DataFormat::kBinaryDataFormat, "bin", src,
        pg::TimestampTz(src)));
    std::string select_timezones = R"~(
        select fmt, notz, tz, notz at time zone current_setting('TIMEZONE'),
          notz at time zone 'UTC', current_setting('TIMEZONE')
        from tstest
        where notz at time zone current_setting('TIMEZONE') <> tz)~";
    pg::ResultSet res{nullptr};
    EXPECT_NO_THROW(res = conn->Execute(select_timezones));
    EXPECT_EQ(0, res.Size())
        << "There should be no records that differ. " << tmp_tz;
    for (auto r : res) {
      std::string fmt, tz_setting;
      pg::TimePoint tp, tptz, tp_curr_tz, tp_utc;
      r.To(fmt, tp, pg::TimestampTz(tptz), pg::TimestampTz(tp_curr_tz),
           pg::TimestampTz(tp_utc), tz_setting);
      EXPECT_TRUE(EqualToMicroseconds(tp, tptz))
          << "Should be seen equal locally. " << tmp_tz;
      ADD_FAILURE() << fmt
                    << ": According to server timestamp without time zone "
                    << FormatToLocal(tp)
                    << " is different from timestamp with time zone "
                    << FormatToLocal(tptz) << " Timestamp without tz at "
                    << tz_setting << " = " << FormatToLocal(tp_curr_tz)
                    << ", at UTC " << FormatToLocal(tp_utc) << " " << tmp_tz;
    }
    EXPECT_NO_THROW(conn->Execute("delete from tstest"));
  }
  EXPECT_NO_THROW(conn->Execute("drop table tstest"));
}

POSTGRE_TEST_P(Timestamp) {
  ASSERT_TRUE(conn.get());
  // static const auto utc = cctz::utc_time_zone();
  // static const auto local_tz = cctz::local_time_zone();

  pg::ResultSet res{nullptr};
  auto now = std::chrono::system_clock::now();
  EXPECT_NO_THROW(res = conn->ExperimentalExecute(
                      "select $1::timestamp, $2::timestamptz",
                      pg::io::DataFormat::kTextDataFormat,
                      pg::ForceTextFormat(now),
                      pg::ForceTextFormat(pg::TimestampTz(now))));
  pg::TimePoint tp;
  pg::TimePoint tptz;
  res.Front().To(tp, pg::TimestampTz(tptz));

  EXPECT_TRUE(EqualToMicroseconds(tp, now)) << "Text reply format, no tz";
  EXPECT_TRUE(EqualToMicroseconds(tptz, now)) << "Text reply format, with tz";
  EXPECT_TRUE(EqualToMicroseconds(tp, tptz)) << "Text reply format";

  tp = pg::TimePoint{};
  tptz = pg::TimePoint{};
  EXPECT_FALSE(EqualToMicroseconds(tp, now)) << "After reset";
  EXPECT_FALSE(EqualToMicroseconds(tptz, now)) << "After reset";

  EXPECT_NO_THROW(
      res = conn->ExperimentalExecute("select $1::timestamp, $2::timestamptz",
                                      pg::io::DataFormat::kBinaryDataFormat,
                                      now, pg::TimestampTz(now)));

  EXPECT_EQ(pg::io::DataFormat::kBinaryDataFormat, res[0][0].GetDataFormat());
  EXPECT_EQ(pg::io::DataFormat::kBinaryDataFormat, res[0][1].GetDataFormat());
  EXPECT_NO_THROW(res.Front().To(tp, pg::TimestampTz(tptz)));
  EXPECT_TRUE(EqualToMicroseconds(tp, now)) << "Binary reply format, no tz";
  EXPECT_TRUE(EqualToMicroseconds(tptz, now)) << "Binary reply format, with tz";
  EXPECT_TRUE(EqualToMicroseconds(tp, tptz)) << "Binary reply format";

  const char* timezones[]{
      "",
      "Europe/Moscow",
      "Europe/London",
      "Europe/Paris",
      "America/New_York",
      "America/Los_Angeles",
      "America/Buenos_Aires",
      "Africa/Cairo",
      "Africa/Dakar",
      "Asia/Tokyo",
      "Asia/Hong_Kong",
      "Asia/Jerusalem",
      "Asia/Katmandu",       // UTC+5:45
      "Australia/Perth",     // UTC+8:00
      "Australia/Eucla",     // UTC+8:45
      "Australia/Darwin",    // UTC+9:30
      "Australia/Brisbane",  // UTC+10:00
      "Australia/Adelaide",  // UTC+10:30
      "Australia/Sydney",    // UTC+11:00
  };
  for (const auto& tz_name : timezones) {
    CheckInTimezone(conn, tz_name);
  }
}

}  // namespace
