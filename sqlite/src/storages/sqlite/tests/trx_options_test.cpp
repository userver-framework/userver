#include <userver/utest/utest.hpp>

#include <gtest/gtest.h>

#include <userver/engine/async.hpp>
#include <userver/engine/condition_variable.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/logging/log.hpp>

#include <userver/storages/sqlite/impl/connection.hpp>
#include <userver/storages/sqlite/infra/connection_ptr.hpp>
#include <userver/storages/sqlite/options.hpp>

#include <storages/sqlite/tests/utils_test.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

namespace {

std::string TestParamNameIsolationLevel(
    const ::testing::TestParamInfo<sqlite::settings::TransactionOptions::IsolationLevel>& info
) {
    return IsolationLevelToString(info.param);
}

class SQLiteTransactionsConcurrentTest
    : public SQLiteParametrizedFixture<SQLiteCustomConnection, settings::TransactionOptions::IsolationLevel> {
    void PreInitialize(const ClientPtr& client) final {
        UEXPECT_NO_THROW(
            client->Execute(OperationType::kReadWrite, "CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)")
        );
    }
};

}  // namespace

UTEST_P_MT(SQLiteTransactionsConcurrentTest, IsolationLevels, 3) {
    settings::SQLiteSettings settings;
    settings.db_path = GetTestDbPath("test.db");
    settings::TransactionOptions::IsolationLevel trx_isolation_lvl = GetParam();
    if (trx_isolation_lvl == settings::TransactionOptions::IsolationLevel::kReadUncommitted) {
        settings.shared_cache = true;
    }
    ClientPtr client;
    UEXPECT_NO_THROW(client = CreateClient(settings));

    engine::Mutex mu;
    engine::ConditionVariable reader_cv;
    engine::ConditionVariable writer_cv;
    bool writer_inserted = false;
    bool reader_selected = false;

    auto writer = engine::AsyncNoSpan([&]() {
        settings::TransactionOptions writer_options{
            trx_isolation_lvl, settings::TransactionOptions::LockingMode::kImmediate};
        Transaction trx{nullptr, {}};

        UEXPECT_NO_THROW(trx = client->Begin(OperationType::kReadWrite, writer_options));
        UEXPECT_NO_THROW(trx.Execute("INSERT INTO test VALUES (NULL, 'dirty_data')").AsExecutionResult());
        std::unique_lock<engine::Mutex> lock(mu);
        writer_inserted = true;
        reader_cv.NotifyAll();
        EXPECT_TRUE(writer_cv.Wait(lock, [&reader_selected] { return reader_selected; }));
        UEXPECT_NO_THROW(trx.Rollback());
    });

    auto reader = engine::AsyncNoSpan([&]() {
        settings::TransactionOptions reader_options{trx_isolation_lvl};
        Transaction trx{nullptr, {}};

        std::unique_lock<engine::Mutex> lock(mu);
        EXPECT_TRUE(reader_cv.Wait(lock, [&writer_inserted] { return writer_inserted; }));
        UEXPECT_NO_THROW(trx = client->Begin(OperationType::kReadOnly, reader_options));
        std::vector<std::string> result;
        UEXPECT_NO_THROW(
            result = trx.Execute("SELECT value FROM test WHERE value = 'dirty_data'").AsVector<std::string>(kFieldTag)
        );
        reader_selected = true;
        writer_cv.NotifyAll();
        if (trx_isolation_lvl == settings::TransactionOptions::IsolationLevel::kSerializable) {
            EXPECT_TRUE(result.empty());
        } else {
            EXPECT_FALSE(result.empty());
        }
    });

    reader.Get();
    writer.Get();

    Transaction trx{nullptr, {}};
    UEXPECT_NO_THROW(trx = client->Begin(OperationType::kReadOnly, {}));
    std::vector<std::string> result;
    UEXPECT_NO_THROW(
        result = trx.Execute("SELECT value FROM test WHERE value = 'dirty_data'").AsVector<std::string>(kFieldTag)
    );
    EXPECT_TRUE(result.empty());
}

INSTANTIATE_UTEST_SUITE_P(
    SQLiteTrxIsolationLevels,
    SQLiteTransactionsConcurrentTest,
    ::testing::Values(
        settings::TransactionOptions::IsolationLevel::kSerializable,
        settings::TransactionOptions::IsolationLevel::kReadUncommitted
    ),
    TestParamNameIsolationLevel
);

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
