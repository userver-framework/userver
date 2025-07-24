#pragma once

/// @brief Utilities for testing logic working with SQLite.

#include <gtest/gtest.h>

#include <userver/engine/task/task_base.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/fs/blocking/write.hpp>

#include <userver/storages/sqlite/client.hpp>
#include <userver/storages/sqlite/impl/statement_base.hpp>
#include <userver/storages/sqlite/operation_types.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

// Auxiliary types for tests
struct Row final {
    std::int32_t id{};
    std::string value;

    bool operator==(const Row& other) const { return std::tie(id, value) == std::tie(other.id, other.value); }
};

using RowTuple = std::tuple<int, std::string>;

// Main fixture for handle temporary database files
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

    void TearDown() override {
        CleanUp(client_);
        SQLiteFixture::TearDown();
    }

    ClientPtr CreateClient(settings::SQLiteSettings settings = {}) {
        client_ = connection_provider_->CreateClient(settings);
        PreInitialize(client_);
        return client_;
    }

private:
    virtual void PreInitialize(const ClientPtr&) {}

    virtual void CleanUp(const ClientPtr&) {}

    std::unique_ptr<ConnectionProvider> connection_provider_;
    ClientPtr client_;
};

// Create sqlite client (set of connection pools) with custom settings
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

// Create sqlite client (set of connection pools) without any inits and checks
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
        settings.shared_cache = true;
        return SQLiteCustomConnection::CreateClient(settings);
    }
};

// Parametrized tests fixture
template <typename ConnectionProvider, typename T>
class SQLiteParametrizedFixture : public SQLiteCompositeFixture<ConnectionProvider>,
                                  public ::testing::WithParamInterface<T> {};

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
