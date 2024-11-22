#pragma once

/// @file userver/storages/sqlite/tests/utils.hpp
/// @brief Utilities for testing logic working with SQLite.

#include <memory>
#include <userver/storages/sqlite.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

// TODO: write a mock function or class to obtain the correct ResultSet from the
// given data

class SQLiteConnection {
 public:
  virtual ~SQLiteConnection() = default;
  virtual ResultSet Execute() { return {}; }
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

class ConnectionWrapper final {
 public:
  ConnectionWrapper();
  ~ConnectionWrapper();

  Connection& operator*() const;
  Connection* operator->() const;

  template <typename... Args>
  ResultSet DefaultExecute(const std::string& query, const Args&... args) const;

 private:
  ConnectionPtr connection_;
};

template <typename... Args>
ResultSet ConnectionWrapper::DefaultExecute(const std::string& query,
                                            const Args&... args) const {
  return connection_->Execute(query, args...);
}

class TmpTable final {
 public:
  explicit TmpTable(std::string_view definition);
  TmpTable(ConnectionWrapper& connection, std::string_view definition);
  ~TmpTable();

  template <typename... Args>
  std::string FormatWithTableName(std::string_view source,
                                  const Args&... args) const;

  template <typename... Args>
  ResultSet DefaultExecute(std::string_view source, const Args&... args);

  ConnectionWrapper& GetConnection() const;

  Transaction Begin(const std::string& name, TransactionOptions options);

 private:
  static constexpr std::string_view kCreateTableQueryTemplate =
      "CREATE TABLE {} ({})";

  void CreateTable(std::string_view definition);

  std::optional<ConnectionWrapper> owned_connection_;
  ConnectionWrapper& connection_;
  std::string table_name_;
};

template <typename... Args>
std::string TmpTable::FormatWithTableName(std::string_view source,
                                          const Args&... args) const {
  return fmt::format(fmt::runtime(source), table_name_, args...);
}

template <typename... Args>
ResultSet TmpTable::DefaultExecute(std::string_view source,
                                   const Args&... args) {
  return connection_.DefaultExecute(FormatWithTableName(source, table_name_),
                                    args...);
}

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
