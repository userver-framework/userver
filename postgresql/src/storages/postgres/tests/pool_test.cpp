#include <storages/postgres/tests/util_test.hpp>

#include <gtest/gtest.h>

#include <utest/utest.hpp>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/dsn.hpp>
#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/pool.hpp>

namespace pg = storages::postgres;

namespace {

void PoolTransaction(pg::ConnectionPool& pool) {
  pg::Transaction trx{nullptr};
  pg::ResultSet res{nullptr};

  // TODO Check idle connection count before and after begin
  EXPECT_NO_THROW(trx = pool.Begin(pg::TransactionOptions{}));
  EXPECT_NO_THROW(res = trx.Execute("select 1"));
  EXPECT_FALSE(res.IsEmpty()) << "Result set is obtained";
  // TODO Check idle connection count before and after commit
  EXPECT_NO_THROW(trx.Commit());
  EXPECT_THROW(trx.Commit(), pg::NotInTransaction);
  EXPECT_NO_THROW(trx.Rollback());
}

}  // namespace

class PostgrePool : public PostgreSQLBase,
                    public ::testing::WithParamInterface<std::string> {
  void ReadParam() override { dsn_ = GetParam(); }

 protected:
  std::string dsn_;
};

INSTANTIATE_TEST_CASE_P(/*empty*/, PostgrePool,
                        ::testing::ValuesIn(GetDsnFromEnv()), DsnToString);

TEST_P(PostgrePool, ConnectionPool) {
  RunInCoro([this] {
    pg::ConnectionPool pool(dsn_, GetTaskProcessor(), 1, 10);
    pg::detail::ConnectionPtr conn;

    EXPECT_NO_THROW(conn = pool.GetConnection())
        << "Obtained connection from pool";
    CheckConnection(std::move(conn));
  });
}

TEST_P(PostgrePool, ConnectionPoolInitiallyEmpty) {
  RunInCoro([this] {
    pg::ConnectionPool pool(dsn_, GetTaskProcessor(), 0, 1);
    pg::detail::ConnectionPtr conn;

    EXPECT_NO_THROW(conn = pool.GetConnection())
        << "Obtained connection from empty pool";
    CheckConnection(std::move(conn));
  });
}

TEST_P(PostgrePool, ConnectionPoolReachedMaxSize) {
  RunInCoro([this] {
    pg::ConnectionPool pool(dsn_, GetTaskProcessor(), 1, 1);
    pg::detail::ConnectionPtr conn;

    EXPECT_NO_THROW(conn = pool.GetConnection())
        << "Obtained connection from pool";
    EXPECT_THROW(pg::detail::ConnectionPtr conn2 = pool.GetConnection(),
                 pg::PoolError)
        << "Pool reached max size";

    CheckConnection(std::move(conn));
  });
}

TEST_P(PostgrePool, PoolInitialSizeExceedMaxSize) {
  RunInCoro([this] {
    EXPECT_THROW(pg::ConnectionPool(dsn_, GetTaskProcessor(), 2, 1),
                 pg::PoolError)
        << "Pool reached max size";
  });
}

TEST_P(PostgrePool, PoolTransaction) {
  RunInCoro([this] {
    pg::ConnectionPool pool(dsn_, GetTaskProcessor(), 1, 10);
    PoolTransaction(pool);
  });
}

TEST_P(PostgrePool, PoolAliveIfConnectionExists) {
  RunInCoro([this] {
    auto pool =
        std::make_unique<pg::ConnectionPool>(dsn_, GetTaskProcessor(), 1, 1);
    pg::detail::ConnectionPtr conn;

    EXPECT_NO_THROW(conn = pool->GetConnection())
        << "Obtained connection from pool";
    pool.reset();
    CheckConnection(std::move(conn));
  });
}

TEST_P(PostgrePool, ConnectionPtrWorks) {
  RunInCoro([this] {
    auto pool =
        std::make_unique<pg::ConnectionPool>(dsn_, GetTaskProcessor(), 2, 2);
    pg::detail::ConnectionPtr conn;

    EXPECT_NO_THROW(conn = pool->GetConnection())
        << "Obtained connection from pool";
    EXPECT_NO_THROW(conn = pool->GetConnection())
        << "Obtained another connection from pool";
    CheckConnection(std::move(conn));

    // We still should have initial count of working connections in the pool
    EXPECT_NO_THROW(conn = pool->GetConnection())
        << "Obtained connection from pool again";
    EXPECT_NO_THROW(conn = pool->GetConnection())
        << "Obtained another connection from pool again";
    pg::detail::ConnectionPtr conn2;
    EXPECT_NO_THROW(conn2 = pool->GetConnection())
        << "Obtained connection from pool one more time";
    pool.reset();
    CheckConnection(std::move(conn));
    CheckConnection(std::move(conn2));
  });
}
