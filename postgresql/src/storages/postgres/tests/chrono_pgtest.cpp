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

void PrintTo(const TimePointTz& ts, std::ostream* os) { *os << ts.GetUnderlying().time_since_epoch().count(); }

}  // namespace storages::postgres

namespace {

const pg::UserTypes types;

template <typename Duration>
std::string
FormatToTZ(const std::chrono::time_point<std::chrono::system_clock, Duration>& tp, const cctz::time_zone& tz) {
    static const std::string ts_format_tz = "%Y-%m-%d %H:%M:%E*S%Ez";
    return cctz::format(ts_format_tz, tp, tz);
}

template <typename Duration>
std::string FormatToUtc(const std::chrono::time_point<std::chrono::system_clock, Duration>& tp) {
    static const auto utc = cctz::utc_time_zone();
    return FormatToTZ(tp, utc);
}

template <typename Duration>
std::string FormatToLocal(const std::chrono::time_point<std::chrono::system_clock, Duration>& tp) {
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
    const std::chrono::system_clock::time_point& rhs
) {
    auto diff = std::chrono::duration_cast<std::chrono::microseconds>(lhs - rhs);
    if (diff == std::chrono::microseconds{0}) {
        return ::testing::AssertionSuccess();
    } else {
        return ::testing::AssertionFailure()
               << "∆ between " << FormatToLocal(lhs) << " and " << FormatToLocal(rhs) << " is " << diff.count() << "µs";
    }
}

TEST(PostgreIO, Chrono) {
    // postgres only supports microsecond resolution
    const pg::TimePointWithoutTz now{
        std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now())};
    pg::test::Buffer buffer;
    UEXPECT_NO_THROW(io::WriteBuffer(types, buffer, now));
    auto fb = pg::test::MakeFieldBuffer(buffer);
    pg::TimePointWithoutTz tgt;
    UEXPECT_NO_THROW(io::ReadBuffer(fb, tgt));
    EXPECT_EQ(now, tgt) << "Parse buffer " << std::string{buffer.begin(), buffer.end()}
                        << ", expected: " << now.GetUnderlying().time_since_epoch().count()
                        << ", got: " << tgt.GetUnderlying().time_since_epoch().count();
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

UTEST_P(PostgreConnection, ChronoTzSample) {
    CheckConnection(GetConn());
    auto& connection = GetConn();

    /// [tz sample]
    namespace pg = storages::postgres;

    // Postgres only supports microsecond resolution
    const auto now = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());

    connection->Execute(R"(
    CREATE TABLE tz_sample(
      with_tz     TIMESTAMP WITH    TIME ZONE,
      without_tz  TIMESTAMP WITHOUT TIME ZONE
    )
  )");

    // No conversion performed
    constexpr auto kInsertQuery = R"(
    INSERT INTO tz_sample(with_tz, without_tz) VALUES($1, $2)
    RETURNING with_tz, without_tz
  )";
    const auto res = connection->Execute(kInsertQuery, pg::TimePointTz{now}, pg::TimePointWithoutTz{now});

    // Both values contain the correct time point
    EXPECT_EQ(res[0][0].As<pg::TimePointTz>().GetUnderlying(), now);
    EXPECT_EQ(res[0][1].As<pg::TimePointWithoutTz>().GetUnderlying(), now);
    /// [tz sample]

    // Repeating above checks with more diagnostics
    EXPECT_EQ(
        std::chrono::time_point_cast<std::chrono::microseconds>(res[0][0].As<pg::TimePointTz>().GetUnderlying())
            .time_since_epoch()
            .count(),
        now.time_since_epoch().count()
    );
    EXPECT_EQ(
        std::chrono::time_point_cast<std::chrono::microseconds>(res[0][1].As<pg::TimePointWithoutTz>().GetUnderlying())
            .time_since_epoch()
            .count(),
        now.time_since_epoch().count()
    );

    connection->Execute("DROP TABLE tz_sample");
}

template <class AsType, class T>
auto ToSeconds(const T& res) {
    const auto underlying = res[0].template As<AsType>().GetUnderlying();
    const auto epoch = underlying.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(epoch).count();
}

UTEST_P(PostgreConnection, ChronoTzConversions) {
    CheckConnection(GetConn());
    auto& connection = GetConn();

    /// [tz skewed]
    namespace pg = storages::postgres;
    const auto tz_offset_res = connection->Execute("select extract(timezone from now())::integer");
    const auto tz_offset = std::chrono::seconds{tz_offset_res[0].As<int>()};

    const auto now = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
    connection->Execute(R"(
    CREATE TABLE tz_conversion_sample(
      with_tz    TIMESTAMP WITH    TIME ZONE,
      without_tz TIMESTAMP WITHOUT TIME ZONE
    )
  )");

    constexpr auto kInsertQuery = R"(
    INSERT INTO tz_conversion_sample(with_tz, without_tz) VALUES($1, $2)
    RETURNING with_tz, without_tz
  )";
    // ERROR! Types missmatch and are implicitly converted:
    // * TimePointWithoutTz is converted on the DB side and TZ substracted.
    // * TimePointTz is converted on the DB side and TZ added.
    const auto res = connection->Execute(kInsertQuery, pg::TimePointWithoutTz{now}, pg::TimePointTz{now});

    // Both values were skewed
    using time_point = std::chrono::system_clock::time_point;
    EXPECT_EQ(res[0][0].As<time_point>() + tz_offset, now);
    EXPECT_EQ(res[0][1].As<time_point>() - tz_offset, now);
    /// [tz skewed]

    // Repeating above checks with more diagnostics
    EXPECT_EQ(
        std::chrono::time_point_cast<std::chrono::microseconds>(res[0][0].As<time_point>() + tz_offset)
            .time_since_epoch()
            .count(),
        now.time_since_epoch().count()
    ) << "Offset is "
      << tz_offset.count();
    EXPECT_EQ(
        std::chrono::time_point_cast<std::chrono::microseconds>(res[0][1].As<time_point>() - tz_offset)
            .time_since_epoch()
            .count(),
        now.time_since_epoch().count()
    ) << "Offset is "
      << tz_offset.count();

    connection->Execute("DROP TABLE tz_conversion_sample");
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
        os << "Client TZ: " << (val.current_tz_.empty() ? "local" : val.current_tz_);
        return os;
    }
};

void CheckInTimezone(pg::detail::ConnectionPtr& conn, const std::string& tz_name = {}) {
    TemporaryTZ tmp_tz{tz_name};
    auto tz = cctz::local_time_zone();
    if (!tz_name.empty()) {
        ASSERT_TRUE(cctz::load_time_zone(tz_name, &tz)) << "cctz load time zone " << tz_name;
        UASSERT_NO_THROW(conn->SetParameter("TimeZone", tz_name, pg::detail::Connection::ParameterScope::kSession))
            << "Set timezone for Postgres " << tmp_tz;
    }  // Else use local timezone

    std::vector<pg::TimePoint> timepoints{
        pg::ClockType::now(),
        ParseUTC("2018-01-01 10:00:00+00"),  // Some time in winter
        ParseUTC("2018-07-01 10:00:00+00")   // Some time in summer, for DST timezones
    };

    UEXPECT_NO_THROW(conn->Execute("create temp table tstest(fmt text, notz timestamp, tz timestamptz)"));
    for (auto src : timepoints) {
        UEXPECT_NO_THROW(conn->Execute(
            "insert into tstest(fmt, notz, tz) values ($1, $2, $3)",
            "bin",
            pg::TimePointWithoutTz{src},
            pg::TimePointTz(src)
        ));
        std::string select_timezones = R"~(
        select fmt, notz, tz, tz at time zone current_setting('TIMEZONE'),
          tz at time zone 'UTC', current_setting('TIMEZONE')
        from tstest
        where notz <> tz at time zone 'UTC')~";
        pg::ResultSet res{nullptr};
        res = conn->Execute(select_timezones);
        UEXPECT_NO_THROW(res = conn->Execute(select_timezones));
        EXPECT_EQ(0, res.Size()) << "There should be no records that differ. " << tmp_tz;
        for (const auto& r : res) {
            std::string fmt;
            std::string tz_setting;
            pg::TimePointWithoutTz tp;
            pg::TimePointWithoutTz tptz;
            pg::TimePointWithoutTz tp_utc;
            pg::TimePointTz tp_curr_tz;
            r.To(fmt, tp, tptz, tp_curr_tz, tp_utc, tz_setting);
            EXPECT_TRUE(EqualToMicroseconds(tp.GetUnderlying(), tptz.GetUnderlying()))
                << "Should be seen equal locally. " << tmp_tz;
            EXPECT_TRUE(EqualToMicroseconds(tp_curr_tz.GetUnderlying(), tptz.GetUnderlying()))
                << "Should be seen equal locally. " << tmp_tz;
            ADD_FAILURE() << fmt << ": According to server timestamp without time zone "
                          << FormatToLocal(tp.GetUnderlying()) << " is different from timestamp with time zone "
                          << FormatToLocal(tptz.GetUnderlying()) << " Timestamp with tz at " << tz_setting << " = "
                          << FormatToLocal(tp_curr_tz.GetUnderlying()) << ", at UTC "
                          << FormatToLocal(tp_utc.GetUnderlying()) << " " << tmp_tz;
        }
        UEXPECT_NO_THROW(conn->Execute("delete from tstest"));
    }
    UEXPECT_NO_THROW(conn->Execute("drop table tstest"));
}

UTEST_P(PostgreConnection, Timestamp) {
    CheckConnection(GetConn());

    pg::ResultSet res{nullptr};
    auto now = std::chrono::system_clock::now();
    pg::TimePointWithoutTz tp;
    pg::TimePointTz tptz;

    UEXPECT_NO_THROW(
        res = GetConn()->Execute(
            "select $1::timestamp, $2::timestamptz", pg::TimePointWithoutTz{now}, pg::TimePointTz{now}
        )
    );

    UEXPECT_NO_THROW(res.Front().To(tp, tptz));
    EXPECT_TRUE(EqualToMicroseconds(tp.GetUnderlying(), now)) << "no tz";
    EXPECT_TRUE(EqualToMicroseconds(tptz.GetUnderlying(), now)) << "with tz";
    EXPECT_TRUE(EqualToMicroseconds(tp.GetUnderlying(), tptz.GetUnderlying()));

    tp = pg::TimePointWithoutTz{};
    tptz = pg::TimePointTz{};
    EXPECT_FALSE(EqualToMicroseconds(tp.GetUnderlying(), now)) << "After reset";
    EXPECT_FALSE(EqualToMicroseconds(tptz.GetUnderlying(), now)) << "After reset";

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
    UASSERT_NO_THROW(GetConn()->SetParameter("TimeZone", tz_name, pg::detail::Connection::ParameterScope::kSession));

    pg::TimePointTz now{std::chrono::system_clock::now()};
    pg::ResultSet res{nullptr};

    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1, $1::text", now));

    auto [tptz, str] = res.Front().As<pg::TimePointTz, std::string>();
    auto tp = ParseUTC(str);
    EXPECT_EQ(tp, utils::UnderlyingValue(tptz))
        << "Expect " << str << " to be equal to " << FormatToLocal(utils::UnderlyingValue(tptz));
    EXPECT_TRUE(EqualToMicroseconds(tp, utils::UnderlyingValue(now)));
    EXPECT_TRUE(EqualToMicroseconds(utils::UnderlyingValue(tptz), utils::UnderlyingValue(now)));
}

UTEST_P(PostgreConnection, TimestampInfinity) {
    CheckConnection(GetConn());
    pg::ResultSet res{nullptr};

    UEXPECT_NO_THROW(res = GetConn()->Execute("select 'infinity'::timestamp, '-infinity'::timestamp"));
    pg::TimePointWithoutTz pos_inf;
    pg::TimePointWithoutTz neg_inf;

    UEXPECT_NO_THROW(res.Front().To(pos_inf, neg_inf));
    EXPECT_EQ(pg::kTimestampPositiveInfinity, pos_inf);
    EXPECT_EQ(pg::kTimestampNegativeInfinity, neg_inf);

    UEXPECT_NO_THROW(
        res = GetConn()->Execute(
            "select $1, $2",
            pg::TimePointTz{pg::kTimestampPositiveInfinity},
            pg::TimePointTz{pg::kTimestampNegativeInfinity}
        )
    );
    UEXPECT_NO_THROW(res.Front().To(pos_inf, neg_inf));
    EXPECT_EQ(pg::kTimestampPositiveInfinity, pos_inf);
    EXPECT_EQ(pg::kTimestampNegativeInfinity, neg_inf);
}

UTEST_P(PostgreConnection, TimestampStored) {
    CheckConnection(GetConn());

    pg::ResultSet res{nullptr};
    auto now = std::chrono::system_clock::now();
    UEXPECT_NO_THROW(
        res = GetConn()->Execute(
            "select $1, $2", pg::ParameterStore{}.PushBack(pg::TimePointWithoutTz{now}).PushBack(pg::TimePointTz{now})
        )
    );

    pg::TimePointWithoutTz tp;
    pg::TimePointTz tptz;
    UEXPECT_NO_THROW(res.Front().To(tp, tptz));
    EXPECT_TRUE(EqualToMicroseconds(tp.GetUnderlying(), now)) << "no tz";
    EXPECT_TRUE(EqualToMicroseconds(tptz.GetUnderlying(), now)) << "with tz";
    EXPECT_TRUE(EqualToMicroseconds(tp.GetUnderlying(), tptz.GetUnderlying()));
}

}  // namespace

USERVER_NAMESPACE_END
