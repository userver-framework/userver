#include <userver/utest/utest.hpp>

#include <gtest/gtest.h>

#include <userver/concurrent/background_task_storage.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/storages/sqlite.hpp>
#include <userver/storages/sqlite/connection.hpp>
#include <userver/storages/sqlite/tests/utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

UTEST_F(SQLiteCustomConnection, NonExistent) {
  // Try to open a non-existing database
  sqlite::SQLiteSettings settings;
  settings.db_name = GetTestDbPath("test.db");
  settings.create_file = false;

  UEXPECT_THROW(CreateConnection(settings), sqlite::SQLiteException)
      << "Connecting to a non-existent database";
}

UTEST_F(SQLiteCustomConnection, CreateOpen) {
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

UTEST_F(SQLiteCustomConnection, InMemory) {
  // Try to open in-memory database
  sqlite::SQLiteSettings settings;
  settings.db_name = ":memory:";

  UEXPECT_NO_THROW(CreateConnection(settings))
      << "Connect to in-memory database";
}

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
