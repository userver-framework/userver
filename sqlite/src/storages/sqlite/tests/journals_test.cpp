#include <userver/utest/utest.hpp>

#include <fmt/core.h>
#include <gtest/gtest.h>

#include <userver/engine/async.hpp>
#include <userver/engine/get_all.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utest/assert_macros.hpp>

#include <storages/sqlite/tests/utils_test.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

namespace {

std::string TestParamNameJournalMode(const ::testing::TestParamInfo<sqlite::settings::SQLiteSettings::JournalMode>& info
) {
    return JournalModeToString(info.param);
}

class SQLiteJournalModesTest
    : public SQLiteParametrizedFixture<SQLiteCustomConnection, settings::SQLiteSettings::JournalMode> {
    void PreInitialize(const ClientPtr& client) final {
        UEXPECT_NO_THROW(
            client->Execute(OperationType::kReadWrite, "CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)")
        );
    }
};

}  // namespace

UTEST_P_MT(SQLiteJournalModesTest, ReadWrite, 10) {
    settings::SQLiteSettings settings;
    settings.db_path = GetTestDbPath("test.db");
    settings.journal_mode = GetParam();
    if (settings.journal_mode == settings::SQLiteSettings::JournalMode::kMemory) {
        settings.shared_cache = true;
    }
    constexpr size_t kReadTaskCount = 10;
    constexpr size_t kWriteTaskCount = 10;
    constexpr size_t kReadWriteTaskCount = 10;
    constexpr size_t kTotalTasksCount = kReadTaskCount + kWriteTaskCount + kReadWriteTaskCount;

    ClientPtr client;
    UEXPECT_NO_THROW(client = CreateClient(settings)) << "Try to create client";

    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(kTotalTasksCount);

    for (size_t i = 0; i < kWriteTaskCount; ++i) {
        tasks.push_back(engine::AsyncNoSpan([&client, i]() {
            UEXPECT_NO_THROW(
                client->Execute(storages::sqlite::OperationType::kReadWrite, "INSERT INTO test VALUES (NULL, 'data')")
            ) << fmt::format("Try to insert: ({0}, data_{0})", i);
        }));
    }
    for (size_t i = 0; i < kReadTaskCount; ++i) {
        tasks.push_back(engine::AsyncNoSpan([&client]() {
            UEXPECT_NO_THROW(
                (client->Execute(storages::sqlite::OperationType::kReadOnly, "SELECT * FROM test").AsVector<RowTuple>())
            ) << "Try to select all";
        }));
    }
    for (size_t i = kWriteTaskCount; i < kWriteTaskCount + kReadWriteTaskCount; ++i) {
        tasks.push_back(engine::AsyncNoSpan([&client, i]() {
            UEXPECT_NO_THROW((client
                                  ->Execute(
                                      storages::sqlite::OperationType::kReadWrite,
                                      "INSERT INTO test VALUES (NULL, 'data') RETURNING *"
                                  )
                                  .AsVector<RowTuple>())
            ) << fmt::format("Try to insert with returning: ({0}, data_{0})", i);
        }));
    }

    engine::GetAll(tasks);
}

INSTANTIATE_UTEST_SUITE_P(
    SQLiteCommonConcurrent,
    SQLiteJournalModesTest,
    ::testing::Values(
        settings::SQLiteSettings::JournalMode::kWal,
        settings::SQLiteSettings::JournalMode::kDelete,
        settings::SQLiteSettings::JournalMode::kTruncate,
        settings::SQLiteSettings::JournalMode::kPersist,
        settings::SQLiteSettings::JournalMode::kMemory,
        settings::SQLiteSettings::JournalMode::kOff
    ),
    TestParamNameJournalMode
);

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
