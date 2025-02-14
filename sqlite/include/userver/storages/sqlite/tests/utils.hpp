#pragma once

/// @file userver/storages/sqlite/tests/utils.hpp
/// @brief Utilities for testing logic working with SQLite.

#include <filesystem>
#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <userver/components/component_fwd.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/storages/sqlite.hpp>
#include <userver/storages/sqlite/connection.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

// TODO: write a mock function or class to obtain the correct ResultSet from the
// given data

namespace fs = std::filesystem;

class MockSQLiteStatement : public impl::StatementBase {
 public:
  MOCK_METHOD(void, Bind, (const int index, const int32_t value));
  MOCK_METHOD(void, Bind, (const int index, const int64_t value));
  MOCK_METHOD(void, Bind, (const int index, const uint32_t value));
  MOCK_METHOD(void, Bind, (const int index, const uint64_t value));
  MOCK_METHOD(void, Bind, (const int index, const double value));
  MOCK_METHOD(void, Bind, (const int index, const std::string& value));
  MOCK_METHOD(void, Bind, (const int index, const std::string_view value));
  MOCK_METHOD(void, Bind, (const int index, const char* value, const int size));
  MOCK_METHOD(void, Bind, (const int index));

  MOCK_METHOD(int, RowsAffected, (), (const, noexcept, override));
  MOCK_METHOD(int, LastInsertRowId, (), (const, noexcept, override));
  MOCK_METHOD(bool, HasNext, (), (const, noexcept, override));
  MOCK_METHOD(bool, IsDone, (), (const, noexcept, override));
  MOCK_METHOD(void, Next, (), (noexcept, override));
  MOCK_METHOD(int, ColumnCount, (), (const, noexcept, override));

  MOCK_METHOD(int32_t, GetInt32Column, (int column),
              (const, noexcept, override));
  MOCK_METHOD(uint32_t, GetUInt32Column, (int column),
              (const, noexcept, override));
  MOCK_METHOD(int64_t, GetInt64Column, (int column),
              (const, noexcept, override));
  MOCK_METHOD(double, GetDoubleColumn, (int column),
              (const, noexcept, override));
  MOCK_METHOD(const char*, GetCStringColumn, (int column),
              (const, noexcept, override));
  MOCK_METHOD(std::string, GetStringColumn, (int column),
              (const, noexcept, override));
  MOCK_METHOD(const void*, GetBlobColumn, (int column),
              (const, noexcept, override));
  MOCK_METHOD(std::vector<uint8_t>, GetBytesColumn, (int column),
              (const, noexcept, override));
};

class SQLiteTest : public ::testing::Test {
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

class SQLiteCustomConnection : public SQLiteTest {
 public:
  ConnectionPtr CreateConnection(storages::sqlite::SQLiteSettings settings) {
    conn_ = std::make_shared<storages::sqlite::Connection>(
        settings, engine::current_task::GetTaskProcessor());
    CheckConnection(conn_);
    return conn_;
  }

  // TODO: Do I need to validate the connection somehow?
  void CheckConnection(const ConnectionPtr& conn) {
    ASSERT_TRUE(conn) << "Expected non-empty connection pointer";
    EXPECT_NO_THROW(conn->Execute("SELECT 42"));
  }

 private:
  ConnectionPtr conn_;
};

class SQLiteInMemoryConnection : public SQLiteCustomConnection {
 public:
  ConnectionPtr CreateConnection() {
    sqlite::SQLiteSettings settings;
    settings.db_name = ":memory:";
    return SQLiteCustomConnection::CreateConnection(settings);
  }
};

class SQLiteInMemoryInitConnection : public SQLiteInMemoryConnection {
 public:
  ConnectionPtr CreateConnection() {
    sqlite::SQLiteSettings settings;
    settings.db_name = ":memory:";
    auto conn = SQLiteInMemoryConnection::CreateConnection();
    conn->Execute("CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)");
    return conn;
  }
};

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
