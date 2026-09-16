#include <gtest/gtest.h>
#include <array>
#include <storages/odbc/detail/transaction_options.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/storages/odbc.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>

#include <userver/storages/odbc/tests/utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::tests {

using namespace std::chrono_literals;

static_assert(requires(Cluster& cluster, CommandControl command_control, TransactionOptions options) {
    cluster.Begin(ClusterHostType::kMaster);
    cluster.Begin(ClusterHostType::kMaster, OptionalCommandControl{});
    cluster.Begin(ClusterHostType::kMaster, std::nullopt);
    cluster.Begin(ClusterHostType::kMaster, command_control);
    cluster.Begin(ClusterHostType::kMaster, options);
    cluster.Begin(ClusterHostType::kMaster, options, {});
    cluster.Begin(ClusterHostType::kMaster, options, std::nullopt);
    cluster.Begin(ClusterHostType::kMaster, options, command_control);
    cluster.Begin(ClusterHostType::kMaster, TransactionOptions{IsolationLevel::kSerializable});
    cluster.Begin(ClusterHostType::kMaster, TransactionOptions{AccessMode::kReadOnly});
});

void ExerciseReadOnlyRequest(Cluster& cluster) {
    try {
        auto read_only = cluster.Begin(
            ClusterHostType::kMaster,
            TransactionOptions{IsolationLevel::kSerializable, AccessMode::kReadOnly}
        );
        read_only.Rollback();
    } catch (const ConnectionError& ex) {
        EXPECT_NE(std::string_view{ex.what()}.find("SQL_ATTR_ACCESS_MODE"), std::string_view::npos);
    }
}

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

UTEST(TransactionOptions, IsolationMaskPreflight) {
    EXPECT_FALSE(detail::IsIsolationSupported(std::nullopt, IsolationLevel::kSerializable));
    EXPECT_FALSE(detail::IsIsolationSupported(SQL_TXN_READ_COMMITTED, IsolationLevel::kSerializable));
    EXPECT_TRUE(detail::IsIsolationSupported(SQL_TXN_SERIALIZABLE, IsolationLevel::kSerializable));
    EXPECT_TRUE(
        detail::IsIsolationSupported(SQL_TXN_READ_COMMITTED | SQL_TXN_SERIALIZABLE, IsolationLevel::kReadCommitted)
    );
    EXPECT_TRUE(detail::IsExactConnectionAttributeValue(SQL_MODE_READ_ONLY, SQL_MODE_READ_ONLY));
    EXPECT_FALSE(detail::IsExactConnectionAttributeValue(SQL_MODE_READ_ONLY, SQL_MODE_READ_WRITE));
}

UTEST(TransactionOptions, AppliesSupportedIsolationLevelsExactly) {
    const auto host_settings = storages::odbc::settings::HostSettings{kDSN, {1, 1}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{host_settings}}, nullptr);

    const std::array cases{
        std::pair{IsolationLevel::kReadUncommitted, "read uncommitted"},
        std::pair{IsolationLevel::kReadCommitted, "read committed"},
        std::pair{IsolationLevel::kRepeatableRead, "repeatable read"},
        std::pair{IsolationLevel::kSerializable, "serializable"},
    };
    for (const auto& [isolation, expected] : cases) {
        auto trx = cluster.Begin(ClusterHostType::kMaster, TransactionOptions{isolation});
        const auto result = trx.Execute("SHOW transaction_isolation");
        ASSERT_EQ(result.Size(), 1);
        EXPECT_EQ(result[0][0].GetString(), expected);
        trx.Commit();
    }
}

UTEST(TransactionOptions, ReadOnlyNeverSilentlyDowngradesAndConnectionIsReusable) {
    const auto host_settings = storages::odbc::settings::HostSettings{kDSN, {1, 1}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{host_settings}}, nullptr);
    cluster.Execute(ClusterHostType::kMaster, "CREATE TEMP TABLE odbc_transaction_options(value INTEGER)");

    /// [ODBC transaction options]
    const TransactionOptions read_write_serializable{
        IsolationLevel::kSerializable,
        AccessMode::kReadWrite,
    };
    auto configured = cluster.Begin(ClusterHostType::kMaster, read_write_serializable);
    const auto result = configured.Execute("SELECT 1");
    ASSERT_EQ(result.Size(), 1);
    configured.Rollback();
    /// [ODBC transaction options]

    // Drivers may either apply SQL_MODE_READ_ONLY exactly or reject/substitute
    // it. Both are valid; continuing silently with READ_WRITE is not.
    ExerciseReadOnlyRequest(cluster);

    auto read_write = cluster.Begin(ClusterHostType::kMaster, TransactionOptions{AccessMode::kReadWrite});
    read_write.Execute("INSERT INTO odbc_transaction_options VALUES (1)");
    read_write.Commit();

    auto defaults = cluster.Begin(ClusterHostType::kMaster, TransactionOptions{});
    defaults.Execute("INSERT INTO odbc_transaction_options VALUES (2)");
    defaults.Commit();

    const auto count = cluster.Execute(ClusterHostType::kMaster, "SELECT COUNT(*) FROM odbc_transaction_options");
    ASSERT_EQ(count.Size(), 1);
    EXPECT_EQ(count[0][0].GetInt64(), 2);
}

UTEST(TransactionOptions, RestoresConnectionAttributesAcrossAllExitPaths) {
    const auto host_settings = storages::odbc::settings::HostSettings{kDSN, {1, 1}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{host_settings}}, nullptr);

    const auto expect_default = [&cluster] {
        auto trx = cluster.Begin(ClusterHostType::kMaster, TransactionOptions{});
        const auto result = trx.Execute("SHOW transaction_isolation");
        EXPECT_EQ(result[0][0].GetString(), "read committed");
        trx.Commit();
    };

    ExerciseReadOnlyRequest(cluster);
    expect_default();

    {
        auto trx = cluster.Begin(
            ClusterHostType::kMaster,
            TransactionOptions{IsolationLevel::kSerializable, AccessMode::kReadWrite}
        );
        EXPECT_EQ(trx.Execute("SHOW transaction_isolation")[0][0].GetString(), "serializable");
        trx.Commit();
    }
    expect_default();

    {
        auto trx = cluster.Begin(
            ClusterHostType::kMaster,
            TransactionOptions{IsolationLevel::kRepeatableRead, AccessMode::kReadWrite}
        );
        EXPECT_EQ(trx.Execute("SHOW transaction_isolation")[0][0].GetString(), "repeatable read");
        trx.Rollback();
    }
    expect_default();

    {
        auto trx = cluster.Begin(
            ClusterHostType::kMaster,
            TransactionOptions{IsolationLevel::kSerializable, AccessMode::kReadWrite}
        );
        EXPECT_EQ(trx.Execute("SHOW transaction_isolation")[0][0].GetString(), "serializable");
        // RAII rollback must restore all connection attributes for the next borrower.
    }
    expect_default();
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

UTEST(Transaction, BindsParameters) {
    auto cluster = MakeCluster();
    auto trx = cluster.Begin(storages::odbc::ClusterHostType::kMaster);

    /// [ODBC transaction parameter binding]
    const auto result = trx.Execute("SELECT ?::text, ?::integer", "quoted ' value", 42);
    /// [ODBC transaction parameter binding]

    ASSERT_EQ(result.Size(), 1);
    EXPECT_EQ(result[0][0].GetString(), "quoted ' value");
    EXPECT_EQ(result[0][1].GetInt32(), 42);
    trx.Commit();
}

UTEST(Transaction, MaterializedResultRemainsReadableAfterCommit) {
    auto cluster = MakeCluster();
    auto trx = cluster.Begin(storages::odbc::ClusterHostType::kMaster);
    const auto result = trx.Execute("SELECT ?::text AS value", "materialized");
    trx.Commit();

    ASSERT_EQ(result.Size(), 1);
    EXPECT_EQ(result.GetFieldName(0), "value");
    EXPECT_EQ(result[0][0].GetString(), "materialized");
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

UTEST(Transaction, CommandControlIsFreshForEachOperation) {
    auto cluster = MakeCluster();
    auto trx = cluster.Begin(
        storages::odbc::ClusterHostType::kMaster,
        storages::odbc::CommandControl{
            .network_timeout = 50ms,
            .statement_timeout = 50ms,
        }
    );

    engine::SleepFor(100ms);
    const auto result = trx.Execute("SELECT 1");
    ASSERT_EQ(result.Size(), 1);
    EXPECT_EQ(result[0][0].GetInt32(), 1);

    engine::SleepFor(100ms);
    UEXPECT_NO_THROW(trx.Commit());
}

UTEST(Transaction, AutoRollbackHasIndependentCleanupDeadline) {
    const auto host_settings = storages::odbc::settings::HostSettings{kDSN, {1, 1}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{host_settings}}, nullptr);

    cluster.Execute(
        storages::odbc::ClusterHostType::kMaster,
        "CREATE TABLE IF NOT EXISTS t_expired_transaction_cleanup(id INT PRIMARY KEY)"
    );
    cluster.Execute(storages::odbc::ClusterHostType::kMaster, "DELETE FROM t_expired_transaction_cleanup");

    {
        auto trx = cluster.Begin(
            storages::odbc::ClusterHostType::kMaster,
            storages::odbc::CommandControl{
                .network_timeout = 50ms,
                .statement_timeout = 50ms,
            }
        );
        trx.Execute("INSERT INTO t_expired_transaction_cleanup(id) VALUES(1)");
        engine::SleepFor(100ms);
        // The transaction command-control lifetime has elapsed, but destructor
        // rollback still gets a separate cleanup budget.
    }

    const auto result =
        cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT COUNT(*) FROM t_expired_transaction_cleanup");
    ASSERT_EQ(result.Size(), 1);
    EXPECT_EQ(result[0][0].GetInt64(), 0);
}

UTEST(PoolDeadline, AcquireTimeoutIsReportedAsOperationInterrupted) {
    const auto host_settings = storages::odbc::settings::HostSettings{kDSN, {1, 1}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{host_settings}}, nullptr);
    auto holder = cluster.Begin(storages::odbc::ClusterHostType::kMaster);

    UEXPECT_THROW(
        cluster.Execute(
            storages::odbc::ClusterHostType::kMaster,
            storages::odbc::CommandControl{
                .network_timeout = 50ms,
                .statement_timeout = 1s,
            },
            "SELECT 1"
        ),
        storages::odbc::OperationInterrupted
    );
    holder.Rollback();
}

UTEST(PoolDeadline, PoolWaitDoesNotConsumeStatementTimeout) {
    const auto host_settings = storages::odbc::settings::HostSettings{kDSN, {1, 1}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{host_settings}}, nullptr);
    auto holder = cluster.Begin(storages::odbc::ClusterHostType::kMaster);

    auto query = utils::Async("odbc-fresh-statement-deadline", [&cluster] {
        return cluster.Execute(
            storages::odbc::ClusterHostType::kMaster,
            storages::odbc::CommandControl{
                .network_timeout = 1s,
                .statement_timeout = 50ms,
            },
            "SELECT 1"
        );
    });
    engine::SleepFor(100ms);
    holder.Rollback();

    const auto result = query.Get();
    ASSERT_EQ(result.Size(), 1);
    EXPECT_EQ(result[0][0].GetInt32(), 1);
}

}  // namespace storages::odbc::tests

USERVER_NAMESPACE_END
