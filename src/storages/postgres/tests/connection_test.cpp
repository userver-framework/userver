#include <storages/postgres/tests/util_test.hpp>

#include <gtest/gtest.h>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/dsn.hpp>
#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/null.hpp>
#include <storages/postgres/pool.hpp>

#include <storages/postgres/io/chrono.hpp>
#include <storages/postgres/io/force_text.hpp>

namespace pg = storages::postgres;

namespace static_test {

struct __no_input_operator {};
static_assert(
    pg::io::traits::detail::HasInputOperator<__no_input_operator>::value ==
        false,
    "Test input metafunction");
static_assert(pg::io::traits::detail::HasInputOperator<int>::value == true,
              "Test input metafunction");
static_assert(
    (pg::io::traits::HasParser<__no_input_operator,
                               pg::io::DataFormat::kTextDataFormat>::value ==
     false),
    "Test has parser metafunction");
static_assert((pg::io::traits::HasParser<
                   int, pg::io::DataFormat::kTextDataFormat>::value == true),
              "Test has parser metafunction");

}  // namespace static_test

namespace {

POSTGRE_TEST_P(SelectOne) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";

  pg::ResultSet res{nullptr};
  EXPECT_NO_THROW(res = conn->Execute("select 1 as val"))
      << "select 1 successfully executes";
  EXPECT_EQ(pg::ConnectionState::kIdle, conn->GetState());
  EXPECT_TRUE((bool)res) << "Result set is obtained";
  EXPECT_EQ(1, res.Size()) << "Result contains 1 row";
  EXPECT_EQ(1, res.FieldCount()) << "Result contains 1 field";

  for (const auto& row : res) {
    EXPECT_EQ(1, row.Size()) << "Row contains 1 field";
    pg::Integer val{0};
    EXPECT_NO_THROW(row.To(val)) << "Extract row data";
    EXPECT_EQ(1, val) << "Correct data extracted";
    EXPECT_NO_THROW(val = row["val"].As<pg::Integer>())
        << "Access field by name";
    EXPECT_EQ(1, val) << "Correct data extracted";
    for (const auto& field : row) {
      EXPECT_FALSE(field.IsNull()) << "Field is not null";
      EXPECT_EQ(1, field.As<pg::Integer>()) << "Correct data extracted";
    }
  }
}

POSTGRE_TEST_P(SelectPlaceholder) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";

  pg::ResultSet res{nullptr};
  EXPECT_NO_THROW(res = conn->Execute("select $1", 42))
      << "select integral placeholder successfully executes";
  EXPECT_EQ(pg::ConnectionState::kIdle, conn->GetState());
  EXPECT_TRUE((bool)res) << "Result set is obtained";
  EXPECT_EQ(1, res.Size()) << "Result contains 1 row";
  EXPECT_EQ(1, res.FieldCount()) << "Result contains 1 field";

  for (const auto& row : res) {
    EXPECT_EQ(1, row.Size()) << "Row contains 1 field";
    for (const auto& field : row) {
      EXPECT_FALSE(field.IsNull()) << "Field is not null";
      EXPECT_EQ(42, field.As<pg::Integer>());
    }
  }

  EXPECT_NO_THROW(res = conn->Execute("select $1", "fooo"))
      << "select text placeholder successfully executes";
  EXPECT_EQ(pg::ConnectionState::kIdle, conn->GetState());
  EXPECT_TRUE((bool)res) << "Result set is obtained";
  EXPECT_EQ(1, res.Size()) << "Result contains 1 row";
  EXPECT_EQ(1, res.FieldCount()) << "Result contains 1 field";

  for (const auto& row : res) {
    EXPECT_EQ(1, row.Size()) << "Row contains 1 field";
    for (const auto& field : row) {
      EXPECT_FALSE(field.IsNull()) << "Field is not null";
      EXPECT_EQ("fooo", field.As<std::string>());
    }
  }
}

POSTGRE_TEST_P(CheckResultset) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";

  pg::ResultSet res{nullptr};
  EXPECT_NO_THROW(res = conn->Execute(
                      "select $1 as str, $2 as int, $3 as float, $4 as double",
                      "foo bar", 42, 3.14f, 6.28))
      << "select four cols successfully executes";
  EXPECT_EQ(pg::ConnectionState::kIdle, conn->GetState());
  EXPECT_TRUE((bool)res) << "Result set is obtained";
  EXPECT_EQ(1, res.Size()) << "Result contains 1 row";
  EXPECT_EQ(4, res.FieldCount()) << "Result contains 4 fields";

  for (const auto& row : res) {
    EXPECT_EQ(4, row.Size()) << "Row contains 4 fields";
    {
      std::string str;
      pg::Integer i;
      float f;
      double d;
      EXPECT_NO_THROW(row.To(str, i, f, d));
      EXPECT_EQ("foo bar", str);
      EXPECT_EQ(42, i);
      EXPECT_EQ(3.14f, f);
      EXPECT_EQ(6.28, d);
    }
    {
      std::string str;
      pg::Integer i;
      float f;
      double d;
      EXPECT_NO_THROW(row.To({"int", "str", "double", "float"}, i, str, d, f));
      EXPECT_EQ("foo bar", str);
      EXPECT_EQ(42, i);
      EXPECT_EQ(3.14f, f);
      EXPECT_EQ(6.28, d);
    }
    {
      std::string str;
      pg::Integer i;
      float f;
      double d;
      EXPECT_NO_THROW(row.To({1, 0, 3, 2}, i, str, d, f));
      EXPECT_EQ("foo bar", str);
      EXPECT_EQ(42, i);
      EXPECT_EQ(3.14f, f);
      EXPECT_EQ(6.28, d);
    }
    {
      std::string str;
      pg::Integer i;
      float f;
      double d;
      EXPECT_NO_THROW((std::tie(str, i, f, d) =
                           row.As<std::string, pg::Integer, float, double>()));
      EXPECT_EQ("foo bar", str);
      EXPECT_EQ(42, i);
      EXPECT_EQ(3.14f, f);
      EXPECT_EQ(6.28, d);
    }
    {
      auto[str, i, f, d] = row.As<std::string, pg::Integer, float, double>();
      EXPECT_EQ("foo bar", str);
      EXPECT_EQ(42, i);
      EXPECT_EQ(3.14f, f);
      EXPECT_EQ(6.28, d);
    }
    {
      auto[str, d] = row.As<std::string, double>({"str", "double"});
      EXPECT_EQ("foo bar", str);
      EXPECT_EQ(6.28, d);
    }
    {
      auto[str, d] = row.As<std::string, double>({0, 3});
      EXPECT_EQ("foo bar", str);
      EXPECT_EQ(6.28, d);
    }
  }
}

POSTGRE_TEST_P(QueryErrors) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";
  pg::ResultSet res{nullptr};
  const std::string temp_table = R"~(
      create temporary table pgtest(
        id integer primary key,
        nn_val integer not null,
        check_val integer check(check_val > 0))
      )~";
  const std::string dependent_table = R"~(
      create temporary table dependent(
        id integer references pgtest(id) on delete restrict)
      )~";
  const std::string insert_pg_test =
      "insert into pgtest(id, nn_val, check_val) values ($1, $2, $3)";

  EXPECT_THROW(res = conn->Execute("elect"), pg::SyntaxError);
  EXPECT_THROW(res = conn->Execute("select foo"), pg::AccessRuleViolation);
  EXPECT_THROW(res = conn->Execute(""), pg::LogicError);

  EXPECT_NO_THROW(conn->Execute(temp_table));
  EXPECT_NO_THROW(conn->Execute(dependent_table));
  EXPECT_THROW(conn->Execute(insert_pg_test, 1, pg::null<int>, pg::null<int>),
               pg::NotNullViolation);
  EXPECT_NO_THROW(conn->Execute(insert_pg_test, 1, 1, pg::null<int>));
  EXPECT_THROW(conn->Execute(insert_pg_test, 1, 1, pg::null<int>),
               pg::UniqueViolation);
  EXPECT_THROW(conn->Execute(insert_pg_test, 2, 1, 0), pg::CheckViolation);
  EXPECT_THROW(conn->Execute("insert into dependent values(3)"),
               pg::ForeignKeyViolation);
  EXPECT_NO_THROW(conn->Execute("insert into dependent values(1)"));
  EXPECT_THROW(conn->Execute("delete from pgtest where id = 1"),
               pg::ForeignKeyViolation);
}

POSTGRE_TEST_P(ManualTransaction) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";
  EXPECT_EQ(pg::ConnectionState::kIdle, conn->GetState());
  EXPECT_NO_THROW(conn->Execute("begin"))
      << "Successfully execute begin statement";
  EXPECT_EQ(pg::ConnectionState::kTranIdle, conn->GetState());
  EXPECT_NO_THROW(conn->Execute("commit"))
      << "Successfully execute commit statement";
  EXPECT_EQ(pg::ConnectionState::kIdle, conn->GetState());
}

POSTGRE_TEST_P(AutoTransaction) {
  ASSERT_TRUE(conn.get());
  pg::ResultSet res{nullptr};

  EXPECT_EQ(pg::ConnectionState::kIdle, conn->GetState());
  {
    pg::Transaction trx(std::move(conn), pg::TransactionOptions{});
    // TODO Delegate state to transaction and test it
    //    EXPECT_EQ(pg::ConnectionState::kTranIdle, conn->GetState());
    //    EXPECT_TRUE(conn->IsInTransaction());
    //    EXPECT_THROW(conn->Begin(pg::TransactionOptions{}, cb),
    //                 pg::AlreadyInTransaction);

    EXPECT_NO_THROW(res = trx.Execute("select 1"));
    //    EXPECT_EQ(pg::ConnectionState::kTranIdle, conn->GetState());
    EXPECT_TRUE((bool)res) << "Result set is obtained";

    EXPECT_NO_THROW(trx.Commit());

    EXPECT_THROW(trx.Commit(), pg::NotInTransaction);
    EXPECT_NO_THROW(trx.Rollback());
  }
}

POSTGRE_TEST_P(RAIITransaction) {
  ASSERT_TRUE(conn.get());
  pg::ResultSet res{nullptr};

  EXPECT_EQ(pg::ConnectionState::kIdle, conn->GetState());
  {
    pg::Transaction trx(std::move(conn), pg::TransactionOptions{});
    // TODO Delegate state to transaction and test it
    //    EXPECT_EQ(pg::ConnectionState::kTranIdle, conn->GetState());
    //    EXPECT_TRUE(conn->IsInTransaction());

    EXPECT_NO_THROW(res = trx.Execute("select 1"));
    //    EXPECT_EQ(pg::ConnectionState::kTranIdle, conn->GetState());
    EXPECT_TRUE((bool)res) << "Result set is obtained";
  }
}

template <typename Duration>
std::string FormatToUtc(
    const std::chrono::time_point<std::chrono::system_clock, Duration>& tp) {
  static const auto utc = cctz::utc_time_zone();
  static const std::string ts_format_tz = "%Y-%m-%d %H:%M:%E*S%Ez";
  return cctz::format(ts_format_tz, tp, utc);
}

::testing::AssertionResult EqualToMicroseconds(
    const std::chrono::system_clock::time_point& lhs,
    const std::chrono::system_clock::time_point& rhs) {
  auto diff = std::chrono::duration_cast<std::chrono::microseconds>(lhs - rhs);
  if (diff == std::chrono::microseconds{0}) {
    return ::testing::AssertionSuccess();
  } else {
    return ::testing::AssertionFailure()
           << "∆ between " << FormatToUtc(lhs) << " and " << FormatToUtc(rhs)
           << " is " << diff.count() << "µs";
  }
}

POSTGRE_TEST_P(Timestamp) {
  using time_point = std::chrono::system_clock::time_point;
  ASSERT_TRUE(conn.get());
  static const auto utc = cctz::utc_time_zone();
  static const auto local_tz = cctz::local_time_zone();

  pg::ResultSet res{nullptr};
  auto now = std::chrono::system_clock::now();
  EXPECT_NO_THROW(res = conn->Execute("select $1::timestamp, $2::timestamptz",
                                      now, pg::TimestampTz(now, utc)));
  time_point tp;
  time_point tptz;
  res.Front().To(tp, pg::TimestampTz(tptz, local_tz));

  EXPECT_TRUE(EqualToMicroseconds(tp, now)) << "Text reply format";
  EXPECT_TRUE(EqualToMicroseconds(tptz, now)) << "Text reply format";
  EXPECT_TRUE(EqualToMicroseconds(tp, tptz)) << "Text reply format";

  tp = time_point{};
  tptz = time_point{};
  EXPECT_FALSE(EqualToMicroseconds(tp, now)) << "After reset";
  EXPECT_FALSE(EqualToMicroseconds(tptz, now)) << "After reset";

  EXPECT_NO_THROW(
      res = conn->ExperimentalExecute("select $1::timestamp, $2::timestamptz",
                                      pg::io::DataFormat::kBinaryDataFormat,
                                      now, pg::TimestampTz(now, utc)));

  EXPECT_EQ(pg::io::DataFormat::kBinaryDataFormat, res[0][0].GetDataFormat());
  EXPECT_EQ(pg::io::DataFormat::kBinaryDataFormat, res[0][1].GetDataFormat());
  EXPECT_NO_THROW(res.Front().To(tp, pg::TimestampTz(tptz)));
  EXPECT_TRUE(EqualToMicroseconds(tp, now)) << "Binary reply format";
  EXPECT_TRUE(EqualToMicroseconds(tptz, now)) << "Binary reply format";
  EXPECT_TRUE(EqualToMicroseconds(tp, tptz)) << "Binary reply format";

  EXPECT_NO_THROW(conn->Execute(
      "create temp table tstest(fmt text, notz timestamp, tz timestamptz)"));
  EXPECT_NO_THROW(conn->ExperimentalExecute(
      "insert into tstest(fmt, notz, tz) values ($1, $2, $3)",
      pg::io::DataFormat::kTextDataFormat, "txt", pg::ForceTextFormat(now),
      pg::ForceTextFormat(pg::TimestampTz(now))));
  EXPECT_NO_THROW(conn->ExperimentalExecute(
      "insert into tstest(fmt, notz, tz) values ($1, $2, $3)",
      pg::io::DataFormat::kBinaryDataFormat, "bin", now, pg::TimestampTz(now)));
  std::string select_timezones = R"~(
      select fmt, notz, tz, notz at time zone current_setting('TIMEZONE'),
        notz at time zone 'UTC', current_setting('TIMEZONE')
      from tstest
      where notz at time zone current_setting('TIMEZONE') <> tz)~";
  EXPECT_NO_THROW(res = conn->Execute(select_timezones));
  EXPECT_EQ(0, res.Size()) << "There should be no records that differ";
  for (auto r : res) {
    std::string fmt;
    std::string tz;
    time_point tpcurrtz;
    time_point tputc;
    r.To(fmt, tp, pg::TimestampTz(tptz), pg::TimestampTz(tpcurrtz),
         pg::TimestampTz(tputc), tz);
    EXPECT_TRUE(EqualToMicroseconds(tp, tptz))
        << "Should be seen equal locally";
    ADD_FAILURE() << "According to server timestamp without time zone "
                  << FormatToUtc(tp)
                  << " is different from timestamp with time zone "
                  << FormatToUtc(tptz) << ". Data written to db in " << fmt
                  << " data format. Timestamp without tz at " << tz << " = "
                  << FormatToUtc(tpcurrtz) << ", at UTC " << FormatToUtc(tputc);
  }
}

}  // namespace

TEST_P(PostgreConnection, Connect) {
  RunInCoro([this] {
    EXPECT_THROW(pg::detail::Connection::Connect("psql://", GetTaskProcessor()),
                 pg::ConnectionFailed)
        << "Fail to connect with invalid DSN";

    {
      pg::detail::ConnectionPtr conn;

      EXPECT_NO_THROW(conn = pg::detail::Connection::Connect(
                          dsn_list_[0], GetTaskProcessor()))
          << "Connect to correct DSN";
      CheckConnection(std::move(conn));
    }
  });
}
