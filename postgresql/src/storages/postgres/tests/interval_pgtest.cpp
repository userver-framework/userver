#include <storages/postgres/tests/util_pgtest.hpp>
#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/storages/postgres/parameter_store.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;
namespace io = pg::io;
namespace tt = io::traits;

namespace {

TEST(Postgres, Intervals) {
  io::detail::Interval iv{1, 1, 1};  // 1 month 1 day 1 µs
  UEXPECT_THROW(iv.GetDuration(), pg::UnsupportedInterval);

  iv = io::detail::Interval{0, 0, 1000000};
  EXPECT_EQ(std::chrono::seconds{1}, iv.GetDuration());
}

UTEST_F(PostgreConnection, InternalIntervalRoundtrip) {
  CheckConnection(conn);
  struct Interval {
    std::string str;
    io::detail::Interval expected;
  };
  Interval intervals[]{
      {"1 sec", {0, 0, 1000000}}, {"1.01 sec", {0, 0, 1010000}},
      {"1 day", {0, 1, 0}},       {"1 month", {1, 0, 0}},
      {"1 year", {12, 0, 0}},     {"1 mon 2 day 3 sec", {1, 2, 3000000}},
  };

  pg::ResultSet res{nullptr};

  for (const auto& interval : intervals) {
    io::detail::Interval r;
    UEXPECT_NO_THROW(res = conn->Execute("select interval '" + interval.str +
                                         "', '" + interval.str + "'"))
        << "Select interval " << interval.str;
    UEXPECT_NO_THROW(res[0][0].To(r));
    EXPECT_EQ(interval.expected, r) << "Compare values for " << interval.str;
  }
}

UTEST_F(PostgreConnection, IntervalRoundtrip) {
  CheckConnection(conn);

  pg::ResultSet res{nullptr};
  UEXPECT_NO_THROW(
      res = conn->Execute("select $1", std::chrono::microseconds{1000}));
  std::chrono::microseconds us;
  std::chrono::milliseconds ms;
  std::chrono::seconds sec;

  UEXPECT_NO_THROW(res[0][0].To(us));
  UEXPECT_NO_THROW(res[0][0].To(ms));
  EXPECT_EQ(std::chrono::microseconds{1000}, us);
  EXPECT_EQ(std::chrono::microseconds{1000}, ms);

  UEXPECT_NO_THROW(res = conn->Execute("select $1", std::chrono::seconds{-1}));
  UEXPECT_NO_THROW(res[0][0].To(us));
  UEXPECT_NO_THROW(res[0][0].To(ms));
  UEXPECT_NO_THROW(res[0][0].To(sec));
  EXPECT_EQ(std::chrono::seconds{-1}, us);
  EXPECT_EQ(std::chrono::seconds{-1}, ms);
  EXPECT_EQ(std::chrono::seconds{-1}, sec);
}

UTEST_F(PostgreConnection, IntervalStored) {
  CheckConnection(conn);

  pg::ResultSet res{nullptr};
  UEXPECT_NO_THROW(
      res = conn->Execute("select $1", pg::ParameterStore{}.PushBack(
                                           std::chrono::microseconds{1000})));
  std::chrono::microseconds us;

  UEXPECT_NO_THROW(res[0][0].To(us));
  EXPECT_EQ(std::chrono::microseconds{1000}, us);

  UEXPECT_NO_THROW(
      res = conn->Execute("select $1", pg::ParameterStore{}.PushBack(
                                           std::chrono::seconds{-1})));
  UEXPECT_NO_THROW(res[0][0].To(us));
  EXPECT_EQ(std::chrono::seconds{-1}, us);
}

}  // namespace

USERVER_NAMESPACE_END
