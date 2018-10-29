#include <storages/postgres/util_test.hpp>

#include <gtest/gtest.h>

#include <utest/utest.hpp>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/dsn.hpp>
#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/pool.hpp>

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

void SelectOne(pg::detail::ConnectionPtr conn) {
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

void SelectPlaceholder(pg::detail::ConnectionPtr conn) {
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

void CheckResultset(pg::detail::ConnectionPtr conn) {
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

void NumericFormat(pg::detail::ConnectionPtr conn) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";
  pg::ResultSet res{nullptr};

  EXPECT_NO_THROW(
      res = conn->ExperimentalExecute("select '3.14'::numeric",
                                      pg::io::DataFormat::kTextDataFormat));
  EXPECT_NO_THROW(
      res = conn->ExperimentalExecute("select '3.14'::numeric",
                                      pg::io::DataFormat::kBinaryDataFormat));
  EXPECT_NO_THROW(
      res = conn->ExperimentalExecute("select '3.14'::numeric(10)",
                                      pg::io::DataFormat::kTextDataFormat));
  EXPECT_NO_THROW(
      res = conn->ExperimentalExecute("select '3.14'::numeric(10)",
                                      pg::io::DataFormat::kBinaryDataFormat));
  EXPECT_NO_THROW(
      res = conn->ExperimentalExecute("select '3.14'::numeric(10, 5)",
                                      pg::io::DataFormat::kTextDataFormat));
  EXPECT_NO_THROW(
      res = conn->ExperimentalExecute("select '3.14'::numeric(10, 5)",
                                      pg::io::DataFormat::kBinaryDataFormat));
  EXPECT_NO_THROW(
      res = conn->ExperimentalExecute("select '0'::numeric(10, 5)",
                                      pg::io::DataFormat::kBinaryDataFormat));
}

void ManualTransaction(pg::detail::ConnectionPtr conn) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";
  EXPECT_EQ(pg::ConnectionState::kIdle, conn->GetState());
  EXPECT_NO_THROW(conn->Execute("begin"))
      << "Successfully execute begin statement";
  EXPECT_EQ(pg::ConnectionState::kTranIdle, conn->GetState());
  EXPECT_NO_THROW(conn->Execute("commit"))
      << "Successfully execute commit statement";
  EXPECT_EQ(pg::ConnectionState::kIdle, conn->GetState());
}

void AutoTransaction(pg::detail::ConnectionPtr conn) {
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
    EXPECT_THROW(trx.Rollback(), pg::NotInTransaction);
  }
}

void RAIITransaction(pg::detail::ConnectionPtr conn) {
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

}  // namespace

class PostgreConnection : public PostgreSQLBase {};

INSTANTIATE_TEST_CASE_P(/*empty*/, PostgreConnection,
                        ::testing::ValuesIn(GetDsnFromEnv()), DsnToString);

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

TEST_P(PostgreConnection, Select1) {
  RunInCoro([this] {
    pg::detail::ConnectionPtr conn;

    EXPECT_NO_THROW(conn = pg::detail::Connection::Connect(dsn_list_[0],
                                                           GetTaskProcessor()))
        << "Connect to correct DSN";
    SelectOne(std::move(conn));
  });
}

TEST_P(PostgreConnection, SelectPlaceholder) {
  RunInCoro([this] {
    pg::detail::ConnectionPtr conn;

    EXPECT_NO_THROW(conn = pg::detail::Connection::Connect(dsn_list_[0],
                                                           GetTaskProcessor()))
        << "Connect to correct DSN";
    SelectPlaceholder(std::move(conn));
  });
}

TEST_P(PostgreConnection, CheckResultSet) {
  RunInCoro([this] {
    pg::detail::ConnectionPtr conn;

    EXPECT_NO_THROW(conn = pg::detail::Connection::Connect(dsn_list_[0],
                                                           GetTaskProcessor()))
        << "Connect to correct DSN";
    CheckResultset(std::move(conn));
  });
}

TEST_P(PostgreConnection, NumericFormat) {
  RunInCoro([this] {
    pg::detail::ConnectionPtr conn;

    EXPECT_NO_THROW(conn = pg::detail::Connection::Connect(dsn_list_[0],
                                                           GetTaskProcessor()))
        << "Connect to correct DSN";
    NumericFormat(std::move(conn));
  });
}

TEST_P(PostgreConnection, ManualTransaction) {
  RunInCoro([this] {
    pg::detail::ConnectionPtr conn;

    EXPECT_NO_THROW(conn = pg::detail::Connection::Connect(dsn_list_[0],
                                                           GetTaskProcessor()))
        << "Connect to correct DSN";
    ManualTransaction(std::move(conn));
  });
}

TEST_P(PostgreConnection, AutoTransaction) {
  RunInCoro([this] {
    pg::detail::ConnectionPtr conn;

    EXPECT_NO_THROW(conn = pg::detail::Connection::Connect(dsn_list_[0],
                                                           GetTaskProcessor()))
        << "Connect to correct DSN";
    AutoTransaction(std::move(conn));
  });
}

TEST_P(PostgreConnection, RAIITransaction) {
  RunInCoro([this] {
    pg::detail::ConnectionPtr conn;

    EXPECT_NO_THROW(conn = pg::detail::Connection::Connect(dsn_list_[0],
                                                           GetTaskProcessor()))
        << "Connect to correct DSN";
    RAIITransaction(std::move(conn));
  });
}
