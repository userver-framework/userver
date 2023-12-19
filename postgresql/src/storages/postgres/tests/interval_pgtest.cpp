#include <storages/postgres/tests/util_pgtest.hpp>
#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/storages/postgres/parameter_store.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

/// [Parameters store sample]
template <class ConnectionPtr>
auto ParameterStoreSample(const ConnectionPtr& conn, std::optional<int> a = {},
                          std::optional<int> b = {}, std::optional<int> c = {},
                          std::optional<int> d = {},
                          std::optional<int> e = {}) {
  storages::postgres::ParameterStore parameters;
  std::string filter;

  auto append_if_has_value = [&](std::string_view name, const auto& value) {
    if (value) {
      auto separator = (parameters.IsEmpty() ? std::string_view{} : " AND ");
      parameters.PushBack(*value);

      // Do NOT put parameters directly into the query! It leads to
      // vulnerabilities and bad performance.
      filter += fmt::format("{}{}=${}", separator, name, parameters.Size());
    }
  };

  append_if_has_value("a", a);
  append_if_has_value("b", b);
  append_if_has_value("c", c);
  append_if_has_value("d", d);
  append_if_has_value("e", e);

  if (parameters.IsEmpty()) {
    throw std::runtime_error("No filters provided");
  }

  return conn->Execute("SELECT x FROM some_table WHERE " + filter, parameters);
}
/// [Parameters store sample]

namespace pg = storages::postgres;
namespace io = pg::io;

TEST(Postgres, Intervals) {
  io::detail::Interval iv{1, 1, 1};  // 1 month 1 day 1 us
  UEXPECT_THROW(iv.GetDuration(), pg::UnsupportedInterval);

  iv = io::detail::Interval{0, 0, 1000000};
  EXPECT_EQ(std::chrono::seconds{1}, iv.GetDuration());
}

UTEST_P(PostgreConnection, InternalIntervalRoundtrip) {
  CheckConnection(GetConn());
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
    UEXPECT_NO_THROW(res =
                         GetConn()->Execute("select interval '" + interval.str +
                                            "', '" + interval.str + "'"))
        << "Select interval " << interval.str;
    UEXPECT_NO_THROW(res[0][0].To(r));
    EXPECT_EQ(interval.expected, r) << "Compare values for " << interval.str;
  }
}

UTEST_P(PostgreConnection, IntervalRoundtrip) {
  CheckConnection(GetConn());

  pg::ResultSet res{nullptr};
  UEXPECT_NO_THROW(
      res = GetConn()->Execute("select $1", std::chrono::microseconds{1000}));
  std::chrono::microseconds us;
  std::chrono::milliseconds ms;
  std::chrono::seconds sec;

  UEXPECT_NO_THROW(res[0][0].To(us));
  UEXPECT_NO_THROW(res[0][0].To(ms));
  EXPECT_EQ(std::chrono::microseconds{1000}, us);
  EXPECT_EQ(std::chrono::microseconds{1000}, ms);

  UEXPECT_NO_THROW(
      res = GetConn()->Execute("select $1", std::chrono::seconds{-1}));
  UEXPECT_NO_THROW(res[0][0].To(us));
  UEXPECT_NO_THROW(res[0][0].To(ms));
  UEXPECT_NO_THROW(res[0][0].To(sec));
  EXPECT_EQ(std::chrono::seconds{-1}, us);
  EXPECT_EQ(std::chrono::seconds{-1}, ms);
  EXPECT_EQ(std::chrono::seconds{-1}, sec);
}

UTEST_P(PostgreConnection, IntervalStored) {
  CheckConnection(GetConn());

  pg::ResultSet res{nullptr};
  UEXPECT_NO_THROW(res = GetConn()->Execute(
                       "select $1", pg::ParameterStore{}.PushBack(
                                        std::chrono::microseconds{1000})));
  std::chrono::microseconds us;

  UEXPECT_NO_THROW(res[0][0].To(us));
  EXPECT_EQ(std::chrono::microseconds{1000}, us);

  UEXPECT_NO_THROW(
      res = GetConn()->Execute("select $1", pg::ParameterStore{}.PushBack(
                                                std::chrono::seconds{-1})));
  UEXPECT_NO_THROW(res[0][0].To(us));
  EXPECT_EQ(std::chrono::seconds{-1}, us);
}

UTEST_P(PostgreConnection, ParamsStoreSample) {
  CheckConnection(GetConn());

  UEXPECT_NO_THROW(
      GetConn()->Execute("create temp table some_table(a integer, b integer, c "
                         "integer, d integer, e integer, x integer)"));

  UEXPECT_NO_THROW(GetConn()->Execute(
      "insert into some_table(a, b, c, d, e, x) values ($1, $2, "
      "$3, $4, $5, $6)",
      1, 2, 3, 4, 5, 6));

  UEXPECT_NO_THROW(GetConn()->Execute(
      "insert into some_table(a, b, c, d, e, x) values ($1, $2, "
      "$3, $4, $5, $6)",
      7, 8, 9, 10, 11, 12));

  EXPECT_EQ(ParameterStoreSample(GetConn(), 1).AsSingleRow<int>(), 6);
  EXPECT_EQ(ParameterStoreSample(GetConn(), 1, 2).AsSingleRow<int>(), 6);
  EXPECT_EQ(ParameterStoreSample(GetConn(), 1, 2, 3).AsSingleRow<int>(), 6);
  EXPECT_EQ(ParameterStoreSample(GetConn(), 1, 2, 3, 4).AsSingleRow<int>(), 6);
  EXPECT_EQ(ParameterStoreSample(GetConn(), 1, 2, 3, 4, 5).AsSingleRow<int>(),
            6);
  EXPECT_EQ(ParameterStoreSample(GetConn(), 1, 2, 3, {}, 5).AsSingleRow<int>(),
            6);
  EXPECT_EQ(ParameterStoreSample(GetConn(), {}, 2, 3, {}, 5).AsSingleRow<int>(),
            6);
  EXPECT_EQ(ParameterStoreSample(GetConn(), 1, {}, 3, {}, 5).AsSingleRow<int>(),
            6);
  EXPECT_EQ(ParameterStoreSample(GetConn(), {}, {}, 3, {}).AsSingleRow<int>(),
            6);
  EXPECT_EQ(ParameterStoreSample(GetConn(), {}, {}, {}, 4).AsSingleRow<int>(),
            6);
}
}  // namespace

USERVER_NAMESPACE_END
