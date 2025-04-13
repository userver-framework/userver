#pragma once

/// @file userver/storages/sqlite/tests/utils.hpp
/// @brief Utilities for testing logic working with SQLite.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <userver/engine/task/task_base.hpp>
#include <userver/fs/blocking/temp_directory.hpp>

#include <userver/storages/sqlite/client.hpp>
#include <userver/storages/sqlite/impl/statement_base.hpp>
#include <userver/storages/sqlite/operation_types.hpp>
#include "userver/fs/blocking/write.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

// Auxiliary types for tests
struct Row final {
    std::int32_t id{};
    std::string value;

    bool operator==(const Row& other) const { return std::tie(id, value) == std::tie(other.id, other.value); }
};

using RowTuple = std::tuple<int, std::string>;

// Mock class for main object in sqlite execution query processs
// Bind -> Execution (step, chech actual status) -> Extract (result set or
// execution result)
class MockSQLiteStatement : public impl::StatementBase {
public:
    MOCK_METHOD(OperationType, GetOperationType, (), (noexcept, const, override));

    MOCK_METHOD(void, Bind, (const int index, const std::int32_t value));
    MOCK_METHOD(void, Bind, (const int index, const std::int64_t value));
    MOCK_METHOD(void, Bind, (const int index, const std::uint32_t value));
    MOCK_METHOD(void, Bind, (const int index, const std::uint64_t value));
    MOCK_METHOD(void, Bind, (const int index, const double value));
    MOCK_METHOD(void, Bind, (const int index, const std::string& value));
    MOCK_METHOD(void, Bind, (const int index, const std::string_view value));
    MOCK_METHOD(void, Bind, (const int index, const char* value, const int size));
    MOCK_METHOD(void, Bind, (const int index, const std::vector<std::uint8_t>& value));
    MOCK_METHOD(void, Bind, (const int index));

    MOCK_METHOD(int, ColumnCount, (), (const, noexcept, override));
    MOCK_METHOD(bool, HasNext, (), (const, noexcept, override));
    MOCK_METHOD(bool, IsDone, (), (const, noexcept, override));
    MOCK_METHOD(bool, IsFail, (), (const, noexcept, override));
    MOCK_METHOD(void, Next, (), (noexcept, override));
    MOCK_METHOD(void, CheckStepStatus, (), (override));

    MOCK_METHOD(std::int64_t, RowsAffected, (), (const, noexcept, override));
    MOCK_METHOD(std::int64_t, LastInsertRowId, (), (const, noexcept, override));
    MOCK_METHOD(bool, IsNull, (int column), (const, noexcept, override));
    MOCK_METHOD(void, Extract, (int column, std::int8_t& val), (const, noexcept, override));
    MOCK_METHOD(void, Extract, (int column, std::uint8_t& val), (const, noexcept, override));
    MOCK_METHOD(void, Extract, (int column, std::int16_t& val), (const, noexcept, override));
    MOCK_METHOD(void, Extract, (int column, std::uint16_t& val), (const, noexcept, override));
    MOCK_METHOD(void, Extract, (int column, std::int32_t& val), (const, noexcept, override));
    MOCK_METHOD(void, Extract, (int column, std::uint32_t& val), (const, noexcept, override));
    MOCK_METHOD(void, Extract, (int column, std::int64_t& val), (const, noexcept, override));
    MOCK_METHOD(void, Extract, (int column, std::uint64_t& val), (const, noexcept, override));
    MOCK_METHOD(void, Extract, (int column, float& val), (const, noexcept, override));
    MOCK_METHOD(void, Extract, (int column, double& val), (const, noexcept, override));
    MOCK_METHOD(void, Extract, (int column, std::string& val), (const, noexcept, override));
    MOCK_METHOD(void, Extract, (int column, std::vector<uint8_t>& val), (const, noexcept, override));
};

// Main fixture for handle tempory database files
// Create test tmp dir on start and delete it on finish
class SQLiteFixture : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = fs::blocking::TempDirectory::Create();
        fs::blocking::Chmod(test_dir_.GetPath(), boost::filesystem::perms::all_all);
    }

    void TearDown() override {}

    std::string GetTestDbPath(const std::string& db_name) const { return test_dir_.GetPath() + "/" + db_name; }

private:
    fs::blocking::TempDirectory test_dir_;
};

template <typename ConnectionProvider>
class SQLiteCompositeFixture : public SQLiteFixture {
public:
    SQLiteCompositeFixture() : connection_provider_(std::make_unique<ConnectionProvider>()) {}

    ~SQLiteCompositeFixture() override = default;

    void SetUp() override { SQLiteFixture::SetUp(); }

    void TearDown() override { SQLiteFixture::TearDown(); }

    ClientPtr CreateClient(settings::SQLiteSettings settings = {}) {
        auto client = connection_provider_->CreateClient(settings);
        PreInitialize(client);
        return client;
    }

private:
    virtual void PreInitialize(const ClientPtr&) {}

    std::unique_ptr<ConnectionProvider> connection_provider_;
};

// Create sqlite client (set of conection pools) with custom settings
class SQLiteCustomConnection {
public:
    ClientPtr CreateClient(settings::SQLiteSettings settings) {
        auto client = std::make_shared<storages::sqlite::Client>(settings, engine::current_task::GetTaskProcessor());
        CheckClient(client);
        return client;
    }

    void CheckClient(const ClientPtr& client) {
        ASSERT_TRUE(client) << "Expected non-empty connection pointer";
        EXPECT_NO_THROW(client->Execute(OperationType::kReadOnly, "SELECT 42")) << "Try execute query";
    }
};

// Create sqlite client (set of conection pools) without any inits and checks
class SQLitePureConnection {
public:
    ClientPtr CreateClient(settings::SQLiteSettings settings) {
        return std::make_shared<storages::sqlite::Client>(settings, engine::current_task::GetTaskProcessor());
    }
};

// Create sqlite client with in-memory shared connections
class SQLiteInMemoryConnection : public SQLiteCustomConnection {
public:
    ClientPtr CreateClient(settings::SQLiteSettings settings = {}) {
        settings.db_path = "file::memory:";
        settings.shared_cashe = true;
        return SQLiteCustomConnection::CreateClient(settings);
    }
};

// Parametrized tests fixture
template <typename ConnectionProvider, typename T>
class SQLiteParametrizedFixture : public SQLiteCompositeFixture<ConnectionProvider>,
                                  public ::testing::WithParamInterface<T> {
public:
    void SetUp() override { SQLiteFixture::SetUp(); }

    void TearDown() override { SQLiteFixture::TearDown(); }
};

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
