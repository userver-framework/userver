#pragma once

/// @file userver/storages/sqlite/tests/utils.hpp
/// @brief Utilities for testing logic working with SQLite.

#include <filesystem>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <userver/engine/task/task_base.hpp>

#include <userver/storages/sqlite/client.hpp>
#include <userver/storages/sqlite/impl/statement_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

namespace fs = std::filesystem;

// Auxiliary types for tests
struct Row final {
  int id{};
  std::string value;

  bool operator==(const Row& other) const {
    return std::tie(id, value) == std::tie(other.id, other.value);
  }
};

using RowTuple = std::tuple<int, std::string>;

// Mock class for main object in sqlite execution query processs
// Bind -> Execution (step, chech actual status) -> Extract (result set or
// execution result)
class MockSQLiteStatement : public impl::StatementBase {
 public:
  MOCK_METHOD(void, Bind, (const int index, const std::int32_t value));
  MOCK_METHOD(void, Bind, (const int index, const std::int64_t value));
  MOCK_METHOD(void, Bind, (const int index, const std::uint32_t value));
  MOCK_METHOD(void, Bind, (const int index, const std::uint64_t value));
  MOCK_METHOD(void, Bind, (const int index, const double value));
  MOCK_METHOD(void, Bind, (const int index, const std::string& value));
  MOCK_METHOD(void, Bind, (const int index, const std::string_view value));
  MOCK_METHOD(void, Bind, (const int index, const char* value, const int size));
  MOCK_METHOD(void, Bind,
              (const int index, const std::vector<std::uint8_t>& value));
  MOCK_METHOD(void, Bind, (const int index));

  MOCK_METHOD(int, ColumnCount, (), (const, noexcept, override));
  MOCK_METHOD(bool, HasNext, (), (const, noexcept, override));
  MOCK_METHOD(bool, IsDone, (), (const, noexcept, override));
  MOCK_METHOD(void, Next, (), (override));

  MOCK_METHOD(int, RowsAffected, (), (const, noexcept, override));
  MOCK_METHOD(int, LastInsertRowId, (), (const, noexcept, override));
  MOCK_METHOD(bool, IsNull, (int column), (const, noexcept, override));
  MOCK_METHOD(void, Extract, (int column, std::int8_t& val),
              (const, noexcept, override));
  MOCK_METHOD(void, Extract, (int column, std::uint8_t& val),
              (const, noexcept, override));
  MOCK_METHOD(void, Extract, (int column, std::int16_t& val),
              (const, noexcept, override));
  MOCK_METHOD(void, Extract, (int column, std::uint16_t& val),
              (const, noexcept, override));
  MOCK_METHOD(void, Extract, (int column, std::int32_t& val),
              (const, noexcept, override));
  MOCK_METHOD(void, Extract, (int column, std::uint32_t& val),
              (const, noexcept, override));
  MOCK_METHOD(void, Extract, (int column, std::int64_t& val),
              (const, noexcept, override));
  MOCK_METHOD(void, Extract, (int column, std::uint64_t& val),
              (const, noexcept, override));
  MOCK_METHOD(void, Extract, (int column, float& val),
              (const, noexcept, override));
  MOCK_METHOD(void, Extract, (int column, double& val),
              (const, noexcept, override));
  MOCK_METHOD(void, Extract, (int column, std::string& val),
              (const, noexcept, override));
  MOCK_METHOD(void, Extract, (int column, std::vector<uint8_t>& val),
              (const, noexcept, override));
};

// Main fixture for handle tempory database files
// Create test tmp dir on start and delete it on finish
class SQLiteFixture : public ::testing::Test {
 protected:
  void SetUp() override {
    test_dir_ = fs::temp_directory_path() / "sqlite_test";
    fs::create_directory(test_dir_);
  }

  void TearDown() override {
    if (fs::exists(test_dir_)) {
      fs::remove_all(test_dir_);
    }
  }

  std::string GetTestDbPath(const std::string& db_name) const {
    return (test_dir_ / db_name).string();
  }

 private:
  fs::path test_dir_;
};

template <typename ConnectionProvider>
class SQLiteCompositeFixture : public SQLiteFixture {
 public:
  SQLiteCompositeFixture()
      : connection_provider_(std::make_unique<ConnectionProvider>()) {}

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
    auto client = std::make_shared<storages::sqlite::Client>(
        settings, engine::current_task::GetTaskProcessor());
    CheckClient(client);
    return client;
  }

  void CheckClient(const ClientPtr& client) {
    ASSERT_TRUE(client) << "Expected non-empty connection pointer";
    EXPECT_NO_THROW(client->Execute(OperationType::kReadOnly, "SELECT 42"))
        << "Try execute query";
  }
};

// Create sqlite client with in-memory shared connections
class SQLiteInMemoryConnection : public SQLiteCustomConnection {
 public:
  ClientPtr CreateClient(settings::SQLiteSettings settings = {}) {
    settings.db_name = "file::memory:";
    settings.shared_cashe = true;
    return SQLiteCustomConnection::CreateClient(settings);
  }
};

// Parametrized tests fixture
template <typename ConnectionProvider, typename T>
class SQLiteParametrizedFixture
    : public SQLiteCompositeFixture<ConnectionProvider>,
      public ::testing::WithParamInterface<T> {
 public:
  void SetUp() override { SQLiteFixture::SetUp(); }

  void TearDown() override { SQLiteFixture::TearDown(); }
};

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
