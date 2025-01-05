#include "userver/storages/sqlite/connection.hpp"
#include <gtest/gtest.h>
#include <userver/utest/utest.hpp>

#include <userver/concurrent/background_task_storage.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/storages/sqlite.hpp>
#include <userver/storages/sqlite/tests/utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

class SQLiteCustomConnectiom : public SQLiteTest {
 public:
  ConnectionPtr CreateConnection(storages::sqlite::SQLiteSettings settings) {
    auto conn = std::make_shared<storages::sqlite::Connection>(
        settings, engine::current_task::GetTaskProcessor());
    CheckConnection(conn);
    return conn;
  }

  void CheckConnection(const ConnectionPtr& conn) {
    ASSERT_TRUE(conn) << "Expected non-empty connection pointer";
    ASSERT_TRUE(conn->getHandle() != nullptr);
  }
};

UTEST_F(SQLiteCustomConnectiom, NonExistent) {
  // Try to open a non-existing database
  sqlite::SQLiteSettings settings;
  settings.db_name = GetTestDbPath("test.db");
  settings.create_file = false;

  UEXPECT_THROW(CreateConnection(settings), sqlite::SQLiteException)
      << "Connecting to a non-existent database";
}

UTEST_F(SQLiteCustomConnectiom, CreateOpen) {
  // Try to open a non-existing database
  sqlite::SQLiteSettings settings;
  settings.db_name = GetTestDbPath("test.db");
  settings.create_file = true;

  UEXPECT_NO_THROW(CreateConnection(settings))
      << "Connect to a non-existent database, but the file will be created "
         "automatically";

  // Try to open existing database
  settings.create_file = false;
  UEXPECT_NO_THROW(CreateConnection(settings))
      << "Connect to a existent database, but the file will be created "
         "automatically";
}

UTEST_F(SQLiteCustomConnectiom, InMemory) {
  // Try to open in-memory database
  sqlite::SQLiteSettings settings;
  settings.db_name = "::memory";

  UEXPECT_NO_THROW(CreateConnection(settings))
      << "Connect to in-memory database";
}

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
