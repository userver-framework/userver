#include <gtest/gtest.h>
#include <userver/storages/odbc.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>

#include <userver/storages/odbc/tests/utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::tests {

UTEST(CreateTransaction, Works) {
    auto cluster = MakeCluster();
    auto trx = cluster.Begin(storages::odbc::ClusterHostType::kMaster);
    trx.Commit();
}

UTEST(CreateTransaction, MultipleDSN) {
    auto cluster = MakeCluster(kMultiDSNSettings);
    auto trx = cluster.Begin(storages::odbc::ClusterHostType::kMaster);
    trx.Commit();
}

UTEST(CreateTransaction, Rollback) {
    auto cluster = MakeCluster();
    auto trx = cluster.Begin(storages::odbc::ClusterHostType::kMaster);
    trx.Rollback();
}

UTEST(Transaction, QueryInTransaction) {
    auto cluster = MakeCluster();
    auto trx = cluster.Begin(storages::odbc::ClusterHostType::kMaster);
    auto result = trx.Execute("SELECT 1");
    EXPECT_EQ(result.Size(), 1);
    EXPECT_FALSE(result.IsEmpty());
    auto row = result[0];
    EXPECT_EQ(row.Size(), 1);
    EXPECT_FALSE(row[0].IsNull());
    EXPECT_EQ(row[0].GetInt32(), 1);
    trx.Commit();
}

UTEST(Transaction, DoubleCommitThrows) {
    auto cluster = MakeCluster();
    auto trx = cluster.Begin(storages::odbc::ClusterHostType::kMaster);
    trx.Commit();
    UEXPECT_THROW(trx.Commit(), storages::odbc::TransactionException);
}

UTEST(Transaction, CommitAfterRollbackThrows) {
    auto cluster = MakeCluster();
    auto trx = cluster.Begin(storages::odbc::ClusterHostType::kMaster);
    trx.Rollback();
    UEXPECT_THROW(trx.Commit(), storages::odbc::TransactionException);
}

UTEST(Transaction, RollbackAfterCommitThrows) {
    auto cluster = MakeCluster();
    auto trx = cluster.Begin(storages::odbc::ClusterHostType::kMaster);
    trx.Commit();
    UEXPECT_THROW(trx.Rollback(), storages::odbc::TransactionException);
}

UTEST(Transaction, QueryAfterCommitThrows) {
    auto cluster = MakeCluster();
    auto trx = cluster.Begin(storages::odbc::ClusterHostType::kMaster);
    trx.Commit();
    UEXPECT_THROW(trx.Execute("SELECT 1"), storages::odbc::TransactionException);
}

UTEST(Transaction, QueryAfterRollbackThrows) {
    auto cluster = MakeCluster();
    auto trx = cluster.Begin(storages::odbc::ClusterHostType::kMaster);
    trx.Rollback();
    UEXPECT_THROW(trx.Execute("SELECT 1"), storages::odbc::TransactionException);
}

UTEST(Transaction, AutoRollbackOnDestruction) {
    auto cluster = MakeCluster();

    cluster.Execute(
        storages::odbc::ClusterHostType::kMaster,
        "CREATE TABLE IF NOT EXISTS t_auto_rollback(id INT PRIMARY KEY)"
    );
    cluster.Execute(storages::odbc::ClusterHostType::kMaster, "DELETE FROM t_auto_rollback");

    {
        auto trx = cluster.Begin(storages::odbc::ClusterHostType::kMaster);
        trx.Execute("INSERT INTO t_auto_rollback(id) VALUES(999)");
        // trx goes out of scope without Commit — should auto-rollback
    }

    auto result =
        cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT id FROM t_auto_rollback WHERE id=999");
    EXPECT_EQ(result.Size(), 0);
}

UTEST(Transaction, PersistDataAfterCommit) {
    auto cluster = MakeCluster();
    auto trx = cluster.Begin(storages::odbc::ClusterHostType::kMaster);
    trx.Execute("CREATE TABLE IF NOT EXISTS t_commit(id INT PRIMARY KEY)");
    trx.Execute("DELETE FROM t_commit");
    trx.Execute("INSERT INTO t_commit(id) VALUES(100)");
    trx.Commit();

    auto check_trx = cluster.Begin(storages::odbc::ClusterHostType::kMaster);
    auto result = check_trx.Execute("SELECT id FROM t_commit WHERE id=100");
    ASSERT_EQ(result.Size(), 1);
    ASSERT_EQ(result[0][0].GetInt32(), 100);
    check_trx.Commit();
}

UTEST(Transaction, RollbackData) {
    auto cluster = MakeCluster();
    cluster
        .Execute(storages::odbc::ClusterHostType::kMaster, "CREATE TABLE IF NOT EXISTS t_rollback(id INT PRIMARY KEY)");
    cluster.Execute(storages::odbc::ClusterHostType::kMaster, "DELETE FROM t_rollback");

    auto trx = cluster.Begin(storages::odbc::ClusterHostType::kMaster);
    trx.Execute("INSERT INTO t_rollback(id) VALUES(200)");
    trx.Execute("INSERT INTO t_rollback(id) VALUES(300)");
    trx.Rollback();

    auto check_trx = cluster.Begin(storages::odbc::ClusterHostType::kMaster);
    auto result = check_trx.Execute("SELECT id FROM t_rollback WHERE id=200");
    ASSERT_EQ(result.Size(), 0);
    check_trx.Commit();
}

UTEST(Transaction, SequentialTransactionsReuseConnection) {
    auto hostSettings = storages::odbc::settings::HostSettings{kDSN, {1, 1}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{hostSettings}}, nullptr);

    cluster.Execute(storages::odbc::ClusterHostType::kMaster, "CREATE TABLE IF NOT EXISTS t_seq(id INT PRIMARY KEY)");
    cluster.Execute(storages::odbc::ClusterHostType::kMaster, "DELETE FROM t_seq");

    for (int i = 0; i < 5; ++i) {
        auto trx = cluster.Begin(storages::odbc::ClusterHostType::kMaster);
        trx.Execute("INSERT INTO t_seq(id) VALUES(" + std::to_string(i) + ")");
        trx.Commit();
    }

    auto result = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT COUNT(*) FROM t_seq");
    EXPECT_EQ(result[0][0].GetInt64(), 5);
}

UTEST(Transaction, PlainQueryAfterTransaction) {
    auto hostSettings = storages::odbc::settings::HostSettings{kDSN, {1, 1}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{hostSettings}}, nullptr);

    cluster.Execute(
        storages::odbc::ClusterHostType::kMaster,
        "CREATE TABLE IF NOT EXISTS t_plain_after(id INT PRIMARY KEY)"
    );
    cluster.Execute(storages::odbc::ClusterHostType::kMaster, "DELETE FROM t_plain_after");

    {
        auto trx = cluster.Begin(storages::odbc::ClusterHostType::kMaster);
        trx.Execute("INSERT INTO t_plain_after(id) VALUES(1)");
        trx.Commit();
    }

    // Plain (non-transactional) insert after a transaction — verifies autocommit is restored
    cluster.Execute(storages::odbc::ClusterHostType::kMaster, "INSERT INTO t_plain_after(id) VALUES(2)");

    auto result = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT COUNT(*) FROM t_plain_after");
    EXPECT_EQ(result[0][0].GetInt64(), 2);
}

}  // namespace storages::odbc::tests

USERVER_NAMESPACE_END
