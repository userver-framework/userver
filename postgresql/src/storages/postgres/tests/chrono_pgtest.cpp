#include <gtest/gtest.h>

#include <cctz/time_zone.h>

#include <storages/postgres/tests/test_buffers.hpp>
#include <storages/postgres/tests/util_pgtest.hpp>
#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/storages/postgres/io/user_types.hpp>
#include <userver/storages/postgres/parameter_store.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;
namespace io = pg::io;

namespace storages::postgres {

void PrintTo(const TimePointTz& ts, std::ostream* os) {
  *os << ts.GetUnderlying().time_since_epoch().count();
}

}  // namespace storages::postgres

namespace {

const pg::UserTypes types;

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

TEST(PostgreIO, Chrono) {
  // postgres only supports microsecond resolution
  auto now = std::chrono::time_point_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now());
  pg::test::Buffer buffer;
  UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, now));
  auto fb = pg::test::MakeFieldBuffer(buffer);
  pg::TimePoint tgt;
  UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt));
  EXPECT_EQ(now, tgt) << "Parse buffer "
                      << std::string{buffer.begin(), buffer.end()}
                      << ", expected: " << now.time_since_epoch().count()
                      << ", got: " << tgt.time_since_epoch().count();
}

TEST(PostgreIO, ChronoTz) {
  auto now = pg::TimePointTz{std::chrono::system_clock::now()};
  pg::test::Buffer buffer;
  UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, now));
  auto fb = pg::test::MakeFieldBuffer(buffer);
  pg::TimePointTz tgt;
  UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt));
  EXPECT_TRUE(EqualToMicroseconds(now.GetUnderlying(), tgt.GetUnderlying()));
}

// RAII class to set TZ environment variable. Not for use with threads.
class TemporaryTZ {
 public:
  TemporaryTZ(const std::string& tz) : current_tz_{tz} {
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    auto* env = std::getenv("TZ");
    if (env) {
      std::string{env}.swap(old_tz_);
    }
    SetTz(tz);
  }
  ~TemporaryTZ() { SetTz(old_tz_); }

 private:
  static void SetTz(const std::string& tz) {
    if (tz.empty()) {
      // NOLINTNEXTLINE(concurrency-mt-unsafe)
      ::unsetenv("TZ");
    } else {
      // NOLINTNEXTLINE(concurrency-mt-unsafe)
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
    UASSERT_NO_THROW(conn->SetParameter(
        "TimeZone", tz_name, pg::detail::Connection::ParameterScope::kSession))
        << "Set timezone for Postgres " << tmp_tz;
  }  // Else use local timezone

  std::vector<pg::TimePoint> timepoints{
      pg::ClockType::now(),
      ParseUTC("2018-01-01 10:00:00+00"),  // Some time in winter
      ParseUTC(
          "2018-07-01 10:00:00+00")  // Some time in summer, for DST timezones
  };

  UEXPECT_NO_THROW(conn->Execute(
      "create temp table tstest(fmt text, notz timestamp, tz timestamptz)"));
  for (auto src : timepoints) {
    UEXPECT_NO_THROW(
        conn->Execute("insert into tstest(fmt, notz, tz) values ($1, $2, $3)",
                      "bin", src, pg::TimePointTz(src)));
    std::string select_timezones = R"~(
        select fmt, notz, tz, tz at time zone current_setting('TIMEZONE'),
          tz at time zone 'UTC', current_setting('TIMEZONE')
        from tstest
        where notz <> tz at time zone 'UTC')~";
    pg::ResultSet res{nullptr};
    res = conn->Execute(select_timezones);
    UEXPECT_NO_THROW(res = conn->Execute(select_timezones));
    EXPECT_EQ(0, res.Size())
        << "There should be no records that differ. " << tmp_tz;
    for (const auto& r : res) {
      std::string fmt;
      std::string tz_setting;
      pg::TimePoint tp;
      pg::TimePoint tptz;
      pg::TimePoint tp_utc;
      pg::TimePointTz tp_curr_tz;
      r.To(fmt, tp, tptz, tp_curr_tz, tp_utc, tz_setting);
      EXPECT_TRUE(EqualToMicroseconds(tp, tptz))
          << "Should be seen equal locally. " << tmp_tz;
      EXPECT_TRUE(EqualToMicroseconds(utils::UnderlyingValue(tp_curr_tz), tptz))
          << "Should be seen equal locally. " << tmp_tz;
      ADD_FAILURE() << fmt
                    << ": According to server timestamp without time zone "
                    << FormatToLocal(tp)
                    << " is different from timestamp with time zone "
                    << FormatToLocal(tptz) << " Timestamp with tz at "
                    << tz_setting << " = "
                    << FormatToLocal(utils::UnderlyingValue(tp_curr_tz))
                    << ", at UTC " << FormatToLocal(tp_utc) << " " << tmp_tz;
    }
    UEXPECT_NO_THROW(conn->Execute("delete from tstest"));
  }
  UEXPECT_NO_THROW(conn->Execute("drop table tstest"));
}

UTEST_P(PostgreConnection, Timestamp) {
  CheckConnection(GetConn());

  pg::ResultSet res{nullptr};
  auto now = std::chrono::system_clock::now();
  pg::TimePoint tp;
  pg::TimePointTz tptz;

  UEXPECT_NO_THROW(
      res = GetConn()->Execute("select $1::timestamp, $2::timestamptz", now,
                               pg::TimePointTz(now)));

  UEXPECT_NO_THROW(res.Front().To(tp, tptz));
  EXPECT_TRUE(EqualToMicroseconds(tp, now)) << "no tz";
  EXPECT_TRUE(EqualToMicroseconds(utils::UnderlyingValue(tptz), now))
      << "with tz";
  EXPECT_TRUE(EqualToMicroseconds(tp, utils::UnderlyingValue(tptz)));

  tp = pg::TimePoint{};
  tptz = pg::TimePointTz{};
  EXPECT_FALSE(EqualToMicroseconds(tp, now)) << "After reset";
  EXPECT_FALSE(EqualToMicroseconds(utils::UnderlyingValue(tptz), now))
      << "After reset";

  const char* timezones[]{
      "",  // local
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
    CheckInTimezone(GetConn(), tz_name);
  }
}

UTEST_P(PostgreConnection, TimestampTz) {
  CheckConnection(GetConn());
  // Make sure we use a time zone different from UTC and MSK
  const std::string tz_name = "Asia/Yekaterinburg";
  TemporaryTZ tmp_tz{tz_name};
  UASSERT_NO_THROW(GetConn()->SetParameter(
      "TimeZone", tz_name, pg::detail::Connection::ParameterScope::kSession));

  pg::TimePointTz now{std::chrono::system_clock::now()};
  pg::ResultSet res{nullptr};

  UEXPECT_NO_THROW(res = GetConn()->Execute("select $1, $1::text", now));

  auto [tptz, str] = res.Front().As<pg::TimePointTz, std::string>();
  auto tp = ParseUTC(str);
  EXPECT_EQ(tp, utils::UnderlyingValue(tptz))
      << "Expect " << str << " to be equal to "
      << FormatToLocal(utils::UnderlyingValue(tptz));
  EXPECT_TRUE(EqualToMicroseconds(tp, utils::UnderlyingValue(now)));
  EXPECT_TRUE(EqualToMicroseconds(utils::UnderlyingValue(tptz),
                                  utils::UnderlyingValue(now)));
}

UTEST_P(PostgreConnection, TimestampInfinity) {
  CheckConnection(GetConn());
  pg::ResultSet res{nullptr};

  UEXPECT_NO_THROW(res = GetConn()->Execute(
                       "select 'infinity'::timestamp, '-infinity'::timestamp"));
  pg::TimePoint pos_inf;
  pg::TimePoint neg_inf;

  UEXPECT_NO_THROW(res.Front().To(pos_inf, neg_inf));
  EXPECT_EQ(pg::kTimestampPositiveInfinity, pos_inf);
  EXPECT_EQ(pg::kTimestampNegativeInfinity, neg_inf);

  UEXPECT_NO_THROW(res = GetConn()->Execute("select $1, $2",
                                            pg::kTimestampPositiveInfinity,
                                            pg::kTimestampNegativeInfinity));
  UEXPECT_NO_THROW(res.Front().To(pos_inf, neg_inf));
  EXPECT_EQ(pg::kTimestampPositiveInfinity, pos_inf);
  EXPECT_EQ(pg::kTimestampNegativeInfinity, neg_inf);
}

UTEST_P(PostgreConnection, TimestampStored) {
  CheckConnection(GetConn());

  pg::ResultSet res{nullptr};
  auto now = std::chrono::system_clock::now();
  UEXPECT_NO_THROW(res = GetConn()->Execute(
                       "select $1, $2", pg::ParameterStore{}
                                            .PushBack(pg::TimePoint{now})
                                            .PushBack(pg::TimePointTz{now})));

  pg::TimePoint tp;
  pg::TimePointTz tptz;
  UEXPECT_NO_THROW(res.Front().To(tp, tptz));
  EXPECT_TRUE(EqualToMicroseconds(tp, now)) << "no tz";
  EXPECT_TRUE(EqualToMicroseconds(utils::UnderlyingValue(tptz), now))
      << "with tz";
  EXPECT_TRUE(EqualToMicroseconds(tp, utils::UnderlyingValue(tptz)));
}

}  // namespace

USERVER_NAMESPACE_END
