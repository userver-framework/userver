#include <userver/utest/utest.hpp>

#include <fmt/core.h>
#include <gtest/gtest.h>

#include <userver/utest/assert_macros.hpp>

#include <storages/sqlite/tests/utils_test.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

namespace {

constexpr std::string_view kSelectAllRows = "SELECT * FROM test";
constexpr std::string_view kSelectOneRow = "SELECT * FROM test WHERE id=1";
constexpr std::string_view kSelectNullRow = "SELECT * FROM test WHERE id=NULL";
constexpr std::string_view kSelectOneField = "SELECT value FROM test WHERE id=1";
constexpr std::string_view kUnexpectedFieldsSelect = "SELECT unexpected_field FROM test";
constexpr std::string_view kSelectNullField = "SELECT value FROM test WHERE id=NULL";
constexpr std::string_view kDatatypeMismatchInsert = "INSERT INTO test VALUES ('third', 3)";

class SQLiteCommonResultSetTest : public SQLiteCompositeFixture<SQLiteInMemoryConnection> {
    void PreInitialize(const ClientPtr& client) final {
        UEXPECT_NO_THROW(
            client->Execute(OperationType::kReadWrite, "CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)")
        );
        UEXPECT_NO_THROW(client->Execute(OperationType::kReadWrite, "INSERT INTO test VALUES (1, 'first')"));
        UEXPECT_NO_THROW(client->Execute(OperationType::kReadWrite, "INSERT INTO test VALUES (2, 'second')"));
    }
};

}  // namespace

UTEST_F(SQLiteCommonResultSetTest, SuccessExecute) {
    auto client = CreateClient();

    // Get result as vector of tuples
    {
        std::vector<RowTuple> actual;
        UEXPECT_NO_THROW(
            actual =
                client->Execute(storages::sqlite::OperationType::kReadOnly, kSelectAllRows.data()).AsVector<RowTuple>()
        );

        EXPECT_EQ(actual.size(), 2);
        EXPECT_EQ(actual[0], std::make_tuple(1, "first"));
        EXPECT_EQ(actual[1], std::make_tuple(2, "second"));
    }

    // Get result as struct
    {
        Row actual;
        UEXPECT_NO_THROW(
            actual =
                client->Execute(storages::sqlite::OperationType::kReadOnly, kSelectOneRow.data()).AsSingleRow<Row>()
        );

        EXPECT_EQ(actual, (Row{1, "first"}));
    }

    // Get empty result as vector of rows
    {
        std::vector<Row> actual;
        UEXPECT_NO_THROW(
            actual = client->Execute(storages::sqlite::OperationType::kReadOnly, kSelectNullRow.data()).AsVector<Row>()
        );

        EXPECT_TRUE(actual.empty());
    }

    // Get result as a single field
    {
        std::string actual;
        UEXPECT_NO_THROW(
            actual = client->Execute(storages::sqlite::OperationType::kReadOnly, kSelectOneField.data())
                         .AsSingleField<std::string>()
        );

        EXPECT_EQ(actual, "first");
    }

    // Select with unexpected types
    // (https://www.sqlite.org/lang_expr.html#castexpr)
    // also have STRICT mode which fix it
    {
        using UnexpectedRowTuple = std::tuple<std::string, int>;
        std::vector<UnexpectedRowTuple> actual;
        UEXPECT_NO_THROW(
            actual = client->Execute(storages::sqlite::OperationType::kReadOnly, kSelectAllRows.data())
                         .AsVector<UnexpectedRowTuple>()
        );
        EXPECT_EQ(actual[0], std::make_tuple("1", 0));
        EXPECT_EQ(actual[1], std::make_tuple("2", 0));
    }
}

UTEST_F(SQLiteCommonResultSetTest, FailureExecute) {
    auto client = CreateClient();

    // Throw exception if try to get set of row as vector of fields
    {
        UEXPECT_THROW(
            client->Execute(storages::sqlite::OperationType::kReadOnly, kSelectAllRows.data())
                .AsVector<std::string>(kFieldTag),
            SQLiteException
        );
    }

    // Throw exception if try to get single row from empty result
    {
        UEXPECT_THROW(
            client->Execute(storages::sqlite::OperationType::kReadOnly, kSelectNullRow.data()).AsSingleRow<Row>(),
            SQLiteException
        );
    }

    // Throw exception if result is empty
    {
        UEXPECT_THROW(
            client->Execute(storages::sqlite::OperationType::kReadOnly, kSelectNullField.data())
                .AsSingleField<std::string>(),
            SQLiteException
        );
    }

    // Select with unexpected fields
    {
        std::vector<RowTuple> actual;
        UEXPECT_THROW(
            actual = client->Execute(storages::sqlite::OperationType::kReadOnly, kUnexpectedFieldsSelect.data())
                         .AsVector<RowTuple>(),
            SQLiteException
        );
    }

    // Insert unexpected fields (datatype mismatch)
    {
        UEXPECT_THROW(
            client->Execute(storages::sqlite::OperationType::kReadOnly, kDatatypeMismatchInsert.data()), SQLiteException
        );
    }
}

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
