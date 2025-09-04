#include <userver/utest/utest.hpp>

#include <fmt/core.h>
#include <gtest/gtest.h>

#include <userver/utest/assert_macros.hpp>

#include <storages/sqlite/tests/utils_test.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

namespace {

class SQLiteCommonTest : public SQLiteCompositeFixture<SQLiteCustomConnection> {};

}  // namespace

UTEST_F(SQLiteCommonTest, ReadWrite) {
    settings::SQLiteSettings settings;
    settings.db_path = GetTestDbPath("test.db");
    settings.create_file = true;

    ClientPtr client;
    UEXPECT_NO_THROW(client = CreateClient(settings));
    UEXPECT_NO_THROW(client->Execute(
        storages::sqlite::OperationType::kReadWrite, "CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)"
    ));
    UEXPECT_NO_THROW(
        client->Execute(storages::sqlite::OperationType::kReadWrite, "INSERT INTO test VALUES (1, 'first') ")
    );
    UEXPECT_NO_THROW(
        client->Execute(storages::sqlite::OperationType::kReadWrite, "INSERT INTO test VALUES (2, 'second')")
    );
    UEXPECT_THROW(
        client->Execute(storages::sqlite::OperationType::kReadOnly, "INSERT INTO test VALUES (3, 'third')"),
        SQLiteException
    );
    UEXPECT_THROW(
        (client->Execute(storages::sqlite::OperationType::kReadOnly, "INSERT INTO test VALUES (3, 'third') RETURNING *")
             .AsVector<RowTuple>()),
        SQLiteException
    );
    UEXPECT_NO_THROW((
        client->Execute(storages::sqlite::OperationType::kReadWrite, "INSERT INTO test VALUES (3, 'third') RETURNING *")
            .AsVector<RowTuple>()
    ));
    UEXPECT_NO_THROW(
        (client->Execute(storages::sqlite::OperationType::kReadOnly, "SELECT * FROM test").AsVector<RowTuple>())
    );
    UEXPECT_NO_THROW(
        (client->Execute(storages::sqlite::OperationType::kReadWrite, "SELECT * FROM test").AsVector<RowTuple>())
    );
}

UTEST_F(SQLiteCommonTest, ReadOnly) {
    {
        settings::SQLiteSettings settings;
        settings.db_path = GetTestDbPath("test.db");
        settings.create_file = true;
        ClientPtr client;
        UEXPECT_NO_THROW(client = CreateClient(settings)) << "Connect to a non-existent database, but the file will be "
                                                             "created "
                                                             "automatically";
        UEXPECT_NO_THROW(client->Execute(
            storages::sqlite::OperationType::kReadWrite, "CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)"
        ));
        UEXPECT_NO_THROW(
            client->Execute(storages::sqlite::OperationType::kReadWrite, "INSERT INTO test VALUES (1, 'first')")
        );
        UEXPECT_NO_THROW(
            client->Execute(storages::sqlite::OperationType::kReadWrite, "INSERT INTO test VALUES (2, 'second')")
        );
    }

    settings::SQLiteSettings settings;
    settings.db_path = GetTestDbPath("test.db");
    settings.create_file = false;
    settings.read_mode = settings::SQLiteSettings::ReadMode::kReadOnly;

    ClientPtr client;
    UEXPECT_NO_THROW(client = CreateClient(settings));
    UEXPECT_THROW(
        (client->Execute(storages::sqlite::OperationType::kReadOnly, "INSERT INTO test VALUES (3, 'third') RETURNING *")
             .AsVector<RowTuple>()),
        SQLiteException
    );
    UEXPECT_THROW(
        (client
             ->Execute(storages::sqlite::OperationType::kReadWrite, "INSERT INTO test VALUES (3, 'third') RETURNING *")
             .AsVector<RowTuple>()),
        SQLiteException
    );
    UEXPECT_NO_THROW(
        (client->Execute(storages::sqlite::OperationType::kReadOnly, "SELECT * FROM test").AsVector<RowTuple>())
    );
    UEXPECT_NO_THROW(
        (client->Execute(storages::sqlite::OperationType::kReadWrite, "SELECT * FROM test").AsVector<RowTuple>())
    );
}

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
