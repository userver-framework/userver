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

class SQLiteConnectionTest : public SQLiteCompositeFixture<SQLiteCustomConnection> {};

}  // namespace

UTEST_F(SQLiteConnectionTest, NonExistent) {
    // Try to open a non-existing database
    settings::SQLiteSettings settings;
    settings.db_path = GetTestDbPath("test.db");
    settings.create_file = false;

    UEXPECT_THROW(CreateClient(settings), sqlite::SQLiteException) << "Connecting to a non-existent database";
}

UTEST_F(SQLiteConnectionTest, CreateOpen) {
    // Try to open a non-existing database
    settings::SQLiteSettings settings;
    settings.db_path = GetTestDbPath("test.db");
    settings.create_file = true;

    UEXPECT_NO_THROW(CreateClient(settings)) << "Connect to a non-existent database, but the file will be created "
                                                "automatically";

    // Try to open existing database
    settings.create_file = false;
    UEXPECT_NO_THROW(CreateClient(settings)) << "Connect to a existent database, but the file will be created "
                                                "automatically";
}

UTEST_F(SQLiteConnectionTest, InMemory) {
    settings::SQLiteSettings settings;
    settings.db_path = ":memory:";

    UEXPECT_NO_THROW(CreateClient(settings)) << "Connect to in-memory database";
}

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
