#include <userver/utest/utest.hpp>

#include <vector>

#include <fmt/core.h>
#include <gtest/gtest.h>

#include <userver/engine/async.hpp>
#include <userver/engine/condition_variable.hpp>
#include <userver/engine/get_all.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/logging/log.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utils/statistics/testing.hpp>

#include <userver/storages/sqlite/infra/pool.hpp>

#include <storages/sqlite/tests/utils_test.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

namespace {

class SQLiteMetricsTest : public SQLiteCompositeFixture<SQLitePureConnection> {
public:
    utils::statistics::Snapshot
    GetStatistics(std::string prefix, std::vector<utils::statistics::Label> require_labels = {}) {
        return utils::statistics::Snapshot{statistics_storage_, std::move(prefix), std::move(require_labels)};
    }

    ~SQLiteMetricsTest() override { statistics_holder_.Unregister(); }

private:
    void PreInitialize(const ClientPtr& client) final {
        client_ = std::move(client);
        statistics_holder_ = statistics_storage_.RegisterWriter("sqlite", [this](utils::statistics::Writer& writer) {
            client_->WriteStatistics(writer);
        });
    }

    ClientPtr client_;
    utils::statistics::Storage statistics_storage_;
    utils::statistics::Entry statistics_holder_;
};

class SQLiteMetricsPoolTest : public SQLiteFixture {
public:
    infra::PoolPtr GetPool(settings::SQLiteSettings settings = {}) {
        settings.db_path = GetTestDbPath("test.db");
        settings.create_file = true;
        return std::make_shared<infra::Pool>(settings, engine::current_task::GetTaskProcessor());
    }
};

}  // namespace

UTEST_F(SQLiteMetricsTest, PoolBasic) {
    settings::SQLiteSettings settings;
    settings.db_path = GetTestDbPath("test.db");
    settings.create_file = true;

    ClientPtr client;
    UEXPECT_NO_THROW(client = CreateClient(settings));
    UEXPECT_NO_THROW(client->Execute(
        storages::sqlite::OperationType::kReadWrite, "CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)"
    ));

    UEXPECT_NO_THROW(
        client->Execute(storages::sqlite::OperationType::kReadWrite, "INSERT INTO test VALUES (1, 'first') ")
    );
    UEXPECT_NO_THROW(
        client->Execute(storages::sqlite::OperationType::kReadWrite, "INSERT INTO test VALUES (2, 'second')")
    );
    UEXPECT_NO_THROW(
        (client->Execute(storages::sqlite::OperationType::kReadOnly, "SELECT * FROM test").AsVector<RowTuple>())
    );
    UEXPECT_NO_THROW(
        (client->Execute(storages::sqlite::OperationType::kReadWrite, "SELECT * FROM test").AsVector<RowTuple>())
    );
    const auto write_connection_stats = GetStatistics("sqlite.connections", {{"connection_pool", "write"}});
    EXPECT_EQ(write_connection_stats.SingleMetric("overload").AsRate(), 0);
    EXPECT_EQ(write_connection_stats.SingleMetric("created").AsRate(), 1);
    EXPECT_EQ(write_connection_stats.SingleMetric("closed").AsRate(), 0);
    EXPECT_EQ(write_connection_stats.SingleMetric("active").AsInt(), 1);
    EXPECT_EQ(write_connection_stats.SingleMetric("busy").AsInt(), 0);

    const auto read_connection_stats = GetStatistics("sqlite.connections", {{"connection_pool", "read"}});
    EXPECT_EQ(read_connection_stats.SingleMetric("overload").AsRate(), 0);
    EXPECT_EQ(read_connection_stats.SingleMetric("created").AsRate(), 5);
    EXPECT_EQ(read_connection_stats.SingleMetric("closed").AsRate(), 0);
    EXPECT_EQ(read_connection_stats.SingleMetric("active").AsInt(), 5);
    EXPECT_EQ(read_connection_stats.SingleMetric("busy").AsInt(), 0);
}

UTEST_F_MT(SQLiteMetricsTest, PoolWriteInProcess, 10) {
    settings::SQLiteSettings settings;
    settings.db_path = GetTestDbPath("test.db");
    settings.create_file = true;

    ClientPtr client;
    UEXPECT_NO_THROW(client = CreateClient(settings));
    UEXPECT_NO_THROW(client->Execute(
        storages::sqlite::OperationType::kReadWrite, "CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)"
    ));

    constexpr size_t kWriteTaskCount = 5;
    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(kWriteTaskCount);

    engine::Mutex mu;
    engine::ConditionVariable blocked_writer_cv;
    engine::ConditionVariable stat_read_cv;
    bool blocked_writer_inserted = false;
    bool stat_read = false;

    auto blocked_writer = engine::AsyncNoSpan([&]() {
        auto trx = client->Begin(OperationType::kReadWrite, {});
        UEXPECT_NO_THROW(trx.Execute("INSERT INTO test VALUES (NULL, 'data')"));
        std::unique_lock<engine::Mutex> lock(mu);
        blocked_writer_inserted = true;
        blocked_writer_cv.NotifyAll();
        EXPECT_TRUE(stat_read_cv.Wait(lock, [&stat_read] { return stat_read; }));
        trx.Commit();
    });

    std::unique_lock<engine::Mutex> lock(mu);
    EXPECT_TRUE(blocked_writer_cv.Wait(lock, [&blocked_writer_inserted] { return blocked_writer_inserted; }));
    lock.unlock();

    for (size_t i = 0; i < kWriteTaskCount; ++i) {
        tasks.push_back(engine::AsyncNoSpan([&client]() {
            UEXPECT_NO_THROW(
                client->Execute(storages::sqlite::OperationType::kReadWrite, "INSERT INTO test VALUES (NULL, 'data')")
            );
        }));
    }

    EXPECT_TRUE(client->Execute(OperationType::kReadOnly, "SELECT * FROM test").AsVector<RowTuple>().empty());

    const auto write_connection_stats = GetStatistics("sqlite.connections", {{"connection_pool", "write"}});
    EXPECT_EQ(write_connection_stats.SingleMetric("overload").AsRate(), 0);
    EXPECT_EQ(write_connection_stats.SingleMetric("created").AsRate(), 1);
    EXPECT_EQ(write_connection_stats.SingleMetric("closed").AsRate(), 0);
    EXPECT_EQ(write_connection_stats.SingleMetric("active").AsInt(), 1);
    EXPECT_EQ(write_connection_stats.SingleMetric("busy").AsInt(), 1);

    lock.lock();
    stat_read = true;
    stat_read_cv.NotifyAll();
    lock.unlock();

    blocked_writer.Get();
    engine::GetAll(tasks);

    const auto after_write_connection_stats = GetStatistics("sqlite.connections", {{"connection_pool", "write"}});
    EXPECT_EQ(after_write_connection_stats.SingleMetric("overload").AsRate(), 0);
    EXPECT_EQ(after_write_connection_stats.SingleMetric("created").AsRate(), 1);
    EXPECT_EQ(after_write_connection_stats.SingleMetric("closed").AsRate(), 0);
    EXPECT_EQ(after_write_connection_stats.SingleMetric("active").AsInt(), 1);
    EXPECT_EQ(after_write_connection_stats.SingleMetric("busy").AsInt(), 0);
}

UTEST_F_MT(SQLiteMetricsTest, PoolReadsInProcess, 10) {
    settings::SQLiteSettings settings;
    settings.db_path = GetTestDbPath("test.db");
    settings.create_file = true;

    ClientPtr client;
    UEXPECT_NO_THROW(client = CreateClient(settings));
    UEXPECT_NO_THROW(client->Execute(
        storages::sqlite::OperationType::kReadWrite, "CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)"
    ));

    UEXPECT_NO_THROW(client->Execute(OperationType::kReadWrite, "INSERT INTO test VALUES (NULL, 'data')"));

    constexpr size_t kReadTaskCount = 13;
    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(kReadTaskCount);

    engine::Mutex mu;
    engine::ConditionVariable blocked_readers_cv;
    engine::ConditionVariable stat_read_cv;
    bool stat_read = false;
    std::atomic_size_t read_trx_start = 0;

    for (size_t i = 0; i < kReadTaskCount; ++i) {
        tasks.push_back(engine::AsyncNoSpan([&]() {
            auto trx = client->Begin(OperationType::kReadOnly, {});
            UEXPECT_NO_THROW(trx.Execute("SELECT * FROM test").AsVector<RowTuple>());
            std::unique_lock<engine::Mutex> lock(mu);
            ++read_trx_start;
            if (read_trx_start == settings.pool_settings.max_pool_size) {
                blocked_readers_cv.NotifyAll();
            }
            EXPECT_TRUE(stat_read_cv.Wait(lock, [&stat_read] { return stat_read; }));
            trx.Commit();
        }));
    }

    std::unique_lock<engine::Mutex> lock(mu);
    EXPECT_TRUE(blocked_readers_cv.Wait(lock, [&] { return read_trx_start == settings.pool_settings.max_pool_size; }));
    lock.unlock();

    const auto read_connection_stats = GetStatistics("sqlite.connections", {{"connection_pool", "read"}});
    EXPECT_EQ(read_connection_stats.SingleMetric("overload").AsRate(), 0);
    EXPECT_EQ(read_connection_stats.SingleMetric("created").AsRate(), settings.pool_settings.max_pool_size);
    EXPECT_EQ(read_connection_stats.SingleMetric("closed").AsRate(), 0);
    EXPECT_EQ(read_connection_stats.SingleMetric("active").AsInt(), settings.pool_settings.max_pool_size);
    EXPECT_EQ(read_connection_stats.SingleMetric("busy").AsInt(), settings.pool_settings.max_pool_size);

    lock.lock();
    stat_read = true;
    stat_read_cv.NotifyAll();
    lock.unlock();

    engine::GetAll(tasks);

    const auto after_read_connection_stats = GetStatistics("sqlite.connections", {{"connection_pool", "read"}});
    EXPECT_EQ(after_read_connection_stats.SingleMetric("overload").AsRate(), 0);
    EXPECT_EQ(after_read_connection_stats.SingleMetric("created").AsRate(), settings.pool_settings.max_pool_size);
    EXPECT_EQ(after_read_connection_stats.SingleMetric("closed").AsRate(), 0);
    EXPECT_EQ(after_read_connection_stats.SingleMetric("active").AsInt(), settings.pool_settings.max_pool_size);
    EXPECT_EQ(after_read_connection_stats.SingleMetric("busy").AsInt(), 0);
}

UTEST_F(SQLiteMetricsPoolTest, ActiveConnections) {
    settings::SQLiteSettings settings;
    settings.pool_settings.initial_pool_size = 1;
    settings.pool_settings.max_pool_size = 10;
    auto pool = GetPool(settings);

    std::vector<storages::sqlite::infra::ConnectionPtr> connections;
    connections.reserve(3);
    for (size_t i = 0; i < 3; ++i) {
        connections.emplace_back(pool->Acquire());
    }

    {
        const auto& stat = pool->GetStatistics();
        size_t busy = stat.connections.acquired.Load().value - stat.connections.released.Load().value;
        size_t active = stat.connections.created.Load().value - stat.connections.closed.Load().value;
        EXPECT_EQ(busy, 3);
        EXPECT_EQ(active, 3);
    }

    connections.clear();

    {
        const auto& stat = pool->GetStatistics();
        size_t busy = stat.connections.acquired.Load().value - stat.connections.released.Load().value;
        size_t active = stat.connections.created.Load().value - stat.connections.closed.Load().value;
        EXPECT_EQ(busy, 0);
        EXPECT_EQ(active, 3);
    }
}

UTEST_F(SQLiteMetricsTest, NoOp) {
    settings::SQLiteSettings settings;
    settings.db_path = GetTestDbPath("test.db");

    ClientPtr client;
    UEXPECT_NO_THROW(client = CreateClient(settings));

    const auto write_queries_stats = GetStatistics("sqlite.queries", {{"connection_pool", "write"}});
    const auto read_queries_stats = GetStatistics("sqlite.queries", {{"connection_pool", "read"}});
    const auto transactions = GetStatistics("sqlite.transactions");

    EXPECT_EQ(write_queries_stats.SingleMetric("total").AsRate(), 0);
    EXPECT_EQ(write_queries_stats.SingleMetric("executed").AsRate(), 0);
    EXPECT_EQ(write_queries_stats.SingleMetric("error").AsRate(), 0);

    EXPECT_EQ(read_queries_stats.SingleMetric("total").AsRate(), 0);
    EXPECT_EQ(read_queries_stats.SingleMetric("executed").AsRate(), 0);
    EXPECT_EQ(read_queries_stats.SingleMetric("error").AsRate(), 0);

    EXPECT_EQ(transactions.SingleMetric("total").AsRate(), 0);
    EXPECT_EQ(transactions.SingleMetric("commit").AsRate(), 0);
    EXPECT_EQ(transactions.SingleMetric("rollback").AsRate(), 0);
}

UTEST_F(SQLiteMetricsTest, QueriesBasic) {
    settings::SQLiteSettings settings;
    settings.db_path = GetTestDbPath("test.db");

    ClientPtr client;
    UEXPECT_NO_THROW(client = CreateClient(settings));

    UEXPECT_NO_THROW(client->Execute(
        storages::sqlite::OperationType::kReadWrite, "CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)"
    ));

    UEXPECT_NO_THROW(
        client->Execute(storages::sqlite::OperationType::kReadWrite, "INSERT INTO test VALUES (?, ?)", 1, "data")
    );
    UEXPECT_NO_THROW(
        (client->Execute(storages::sqlite::OperationType::kReadOnly, "SELECT * FROM test").AsVector<RowTuple>())
    );
    UEXPECT_NO_THROW(
        (client->Execute(storages::sqlite::OperationType::kReadWrite, "SELECT * FROM test").AsVector<RowTuple>())
    );

    const auto write_snapshot = GetStatistics("sqlite.connections", {{"connection_pool", "write"}});
    EXPECT_EQ(write_snapshot.SingleMetric("active").AsInt(), 1);
    const auto read_snapshot = GetStatistics("sqlite.connections", {{"connection_pool", "read"}});
    EXPECT_EQ(write_snapshot.SingleMetric("active").AsInt(), 1);

    // unsuccessful mutation in prepare time
    UEXPECT_THROW(
        client->Execute(
            storages::sqlite::OperationType::kReadWrite, "INSERT INTO unknown_table VALUES (?, ?)", 2, "data"
        ),
        SQLiteException
    );
    // unsuccessful mutation in runtime
    UEXPECT_THROW(
        client->Execute(storages::sqlite::OperationType::kReadWrite, "INSERT INTO test VALUES (?, ?)", 1, "data"),
        SQLiteException
    );
    // unsuccessful select
    UEXPECT_THROW(client->Execute(OperationType::kReadOnly, "SELECT * FROM unknown_table"), SQLiteException);

    const auto write_connection_stats = GetStatistics("sqlite.connections", {{"connection_pool", "write"}});
    const auto read_connection_stats = GetStatistics("sqlite.connections", {{"connection_pool", "read"}});
    const auto write_queries_stats = GetStatistics("sqlite.queries", {{"connection_pool", "write"}});
    const auto read_queries_stats = GetStatistics("sqlite.queries", {{"connection_pool", "read"}});

    EXPECT_EQ(write_connection_stats.SingleMetric("closed").AsRate(), 0);
    EXPECT_EQ(read_connection_stats.SingleMetric("closed").AsRate(), 0);

    // create + success insert + 2 fail insert (1 in prepare time + 1 in runtime)
    // + select on connection pool (consider all requests without separation by
    // type of operation)
    EXPECT_EQ(write_queries_stats.SingleMetric("total").AsRate(), 5);
    EXPECT_EQ(write_queries_stats.SingleMetric("executed").AsRate(), 3);
    EXPECT_EQ(write_queries_stats.SingleMetric("error").AsRate(), 2);

    // success select on read connection + fail select
    EXPECT_EQ(read_queries_stats.SingleMetric("total").AsRate(), 2);
    EXPECT_EQ(read_queries_stats.SingleMetric("executed").AsRate(), 1);
    EXPECT_EQ(read_queries_stats.SingleMetric("error").AsRate(), 1);
}

UTEST_F(SQLiteMetricsTest, TransactionsBasic) {
    settings::SQLiteSettings settings;
    settings.db_path = GetTestDbPath("test.db");

    ClientPtr client;
    UEXPECT_NO_THROW(client = CreateClient(settings));
    UEXPECT_NO_THROW(client->Execute(
        storages::sqlite::OperationType::kReadWrite, "CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)"
    ));

    constexpr size_t kCommittedWriteTransactions = 5;
    constexpr size_t kRollbackedWriteTransactions = 5;
    constexpr size_t kCommittedReadTransactions = 5;
    constexpr size_t kRollbackedReadTransactions = 5;
    constexpr size_t kTotalCommittedTransactions = kCommittedWriteTransactions + kCommittedReadTransactions;
    constexpr size_t kTotalRollbackedTransactions = kRollbackedReadTransactions + kRollbackedWriteTransactions;
    constexpr size_t kTotalTransactions = kTotalCommittedTransactions + kTotalRollbackedTransactions;

    for (size_t i = 0; i < kCommittedWriteTransactions; ++i) {
        auto trx = client->Begin(storages::sqlite::OperationType::kReadWrite, {});
        UEXPECT_NO_THROW(trx.Execute("INSERT INTO test VALUES (?, ?)", static_cast<std::uint64_t>(i), "data"));
        trx.Commit();
    }

    for (size_t i = 0; i < kRollbackedWriteTransactions; ++i) {
        auto trx = client->Begin(storages::sqlite::OperationType::kReadWrite, {});
        UEXPECT_THROW(
            trx.Execute("INSERT INTO test VALUES (?, ?)", static_cast<std::uint64_t>(i), "data"), SQLiteException
        );
    }

    for (size_t i = 0; i < kCommittedReadTransactions; ++i) {
        auto trx = client->Begin(storages::sqlite::OperationType::kReadOnly, {});
        UEXPECT_NO_THROW((trx.Execute("SELECT * FROM test").AsVector<RowTuple>()));
        trx.Commit();
    }

    for (size_t i = 0; i < kCommittedReadTransactions; ++i) {
        auto trx = client->Begin(storages::sqlite::OperationType::kReadOnly, {});
        UEXPECT_NO_THROW((trx.Execute("SELECT * FROM test").AsVector<RowTuple>()));
        trx.Rollback();
    }

    const auto transactions_stats = GetStatistics("sqlite.transactions");

    EXPECT_EQ(transactions_stats.SingleMetric("total").AsRate(), kTotalTransactions);
    EXPECT_EQ(transactions_stats.SingleMetric("commit").AsRate(), kTotalCommittedTransactions);
    EXPECT_EQ(transactions_stats.SingleMetric("rollback").AsRate(), kTotalRollbackedTransactions);
}

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
