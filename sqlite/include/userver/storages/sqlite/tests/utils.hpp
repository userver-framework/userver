#pragma once

/// @file userver/storages/sqlite/tests/utils.hpp
/// @brief Utilities for testing logic working with SQLite.

#include <filesystem>
#include <memory>

#include <userver/storages/sqlite.hpp>

#include <userver/components/component_fwd.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/statistics/writer.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

// TODO: write a mock function or class to obtain the correct ResultSet from the
// given data

namespace fs = std::filesystem;

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

class SQLiteConnection {
 public:
  virtual ~SQLiteConnection() = default;
  virtual ResultSet Execute() { return {}; }
};

class MockSQLite : public components::ISQLite {
 public:
  MOCK_METHOD(storages::sqlite::ConnectionPtr, GetConnection, (),
              (const, override));
};

class MockSQLiteConnection {
 public:
  MOCK_METHOD(ResultSet, Execute, (), ());
};

template <typename Connection>
class SQLiteTestFixture : public ::testing::Test {
 private:
  std::shared_ptr<MockSQLiteConnection> mock_connection_;

 protected:
  SQLiteTestFixture()
      : mock_connection_(std::make_shared<MockSQLiteConnection>()) {}

  std::shared_ptr<MockSQLiteConnection> GetMockConnection() {
    return mock_connection_;
  }

  ResultSet Execute() { return mock_connection_->Execute(); }
};

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
