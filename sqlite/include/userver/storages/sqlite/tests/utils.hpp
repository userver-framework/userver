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
    // ASSERT_TRUE(conn->getHandle() != nullptr); TODO: need more informative
    // methods
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
