#include <gtest/gtest.h>
#include <userver/storages/odbc.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>

#include <userver/storages/odbc/tests/utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::tests {
UTEST(CreateTransaction, Works) {
    storages::odbc::Cluster cluster(kSettings);
    auto trx = cluster.Begin();
    trx.Commit();
}

UTEST(CreateTransaction, MultipleDSN) {
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{kHostSettings, kHostSettings}});
    auto trx = cluster.Begin();
    trx.Commit();
}

UTEST(CreateTransaction, Rollback) {
    storages::odbc::Cluster cluster(kSettings);
    auto trx = cluster.Begin();
    trx.Rollback();
}

UTEST(Query, Works) {
    storages::odbc::Cluster cluster(kSettings);
    auto trx = cluster.Begin();
    auto result = trx.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
    EXPECT_EQ(result.Size(), 1);
    EXPECT_FALSE(result.IsEmpty());
    auto row = result[0];
    EXPECT_EQ(row.Size(), 1);
    EXPECT_FALSE(row[0].IsNull());
    EXPECT_EQ(row[0].GetInt32(), 1);
    trx.Commit();
}

UTEST(Transaction, DoubleCommitThrows) {
    storages::odbc::Cluster cluster(kSettings);
    auto trx = cluster.Begin();
    trx.Commit();
    EXPECT_THROW(trx.Commit(), storages::odbc::TransactionException);
}

UTEST(Transaction, CommitAfterRollbackThrows) {
    storages::odbc::Cluster cluster(kSettings);
    auto trx = cluster.Begin();
    trx.Rollback();
    EXPECT_THROW(trx.Commit(), storages::odbc::TransactionException);
}

UTEST(Transaction, RollbackAfterCommitThrows) {
    storages::odbc::Cluster cluster(kSettings);
    auto trx = cluster.Begin();
    trx.Commit();
    EXPECT_THROW(trx.Rollback(), storages::odbc::TransactionException);
}

UTEST(Transaction, QueryAfterCommitThrows) {
    storages::odbc::Cluster cluster(kSettings);
    auto trx = cluster.Begin();
    trx.Commit();
    EXPECT_THROW(
        trx.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1"),
        storages::odbc::TransactionException
    );
}

UTEST(Transaction, QueryAfterRollbackThrows) {
    storages::odbc::Cluster cluster(kSettings);
    auto trx = cluster.Begin();
    trx.Rollback();
    EXPECT_THROW(
        trx.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1"),
        storages::odbc::TransactionException
    );
}

UTEST(Transaction, PersistDataAfterCommit) {
    storages::odbc::Cluster cluster(kSettings);
    // Create a table in a transaction and insert a value.
    auto trx = cluster.Begin();
    trx.Execute(storages::odbc::ClusterHostType::kMaster, "CREATE TABLE IF NOT EXISTS t_commit(id INT PRIMARY KEY)");
    trx.Execute(storages::odbc::ClusterHostType::kMaster, "DELETE FROM t_commit");
    trx.Execute(storages::odbc::ClusterHostType::kMaster, "INSERT INTO t_commit(id) VALUES(100)");
    trx.Commit();

    // Check in another transaction that the value exists.
    auto check_trx = cluster.Begin();
    auto result = check_trx.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT id FROM t_commit WHERE id=100");
    ASSERT_EQ(result.Size(), 1);
    ASSERT_EQ(result[0][0].GetInt32(), 100);
    check_trx.Commit();
}

UTEST(Transaction, RollbackData) {
    storages::odbc::Cluster cluster(kSettings);
    // Create a table in a transaction and insert a value, then rollback.
    auto trx = cluster.Begin();
    trx.Execute(storages::odbc::ClusterHostType::kMaster, "CREATE TABLE IF NOT EXISTS t_rollback(id INT PRIMARY KEY)");
    trx.Execute(storages::odbc::ClusterHostType::kMaster, "DELETE FROM t_rollback");
    trx.Execute(storages::odbc::ClusterHostType::kMaster, "INSERT INTO t_rollback(id) VALUES(200)");
    trx.Rollback();

    // Check in another transaction that the value does not exist.
    auto check_trx = cluster.Begin();
    auto result = check_trx.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT id FROM t_rollback WHERE id=200");
    ASSERT_EQ(result.Size(), 0);
    check_trx.Commit();
}

} // namespace storages::odbc::tests

USERVER_NAMESPACE_END
