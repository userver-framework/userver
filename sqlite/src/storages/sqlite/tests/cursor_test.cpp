#include <userver/utest/utest.hpp>

#include <fmt/core.h>
#include <gtest/gtest.h>

#include <userver/utest/assert_macros.hpp>
#include <userver/utils/uuid4.hpp>

#include <storages/sqlite/tests/utils_test.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

namespace {

class SQLiteCursorTest : public SQLiteCompositeFixture<SQLiteInMemoryConnection> {
    void PreInitialize(const ClientPtr& client) final {
        UEXPECT_NO_THROW(
            client->Execute(OperationType::kReadWrite, "CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)")
        );
    }
};

}  // namespace

UTEST_F(SQLiteCursorTest, Works) {
    ClientPtr client;
    UEXPECT_NO_THROW(client = CreateClient()) << "Connect to in-memory database";

    constexpr std::size_t rows_count = 20;
    std::vector<Row> rows_to_insert;
    rows_to_insert.reserve(rows_count);

    for (std::size_t i = 0; i < rows_count; ++i) {
        rows_to_insert.push_back({static_cast<std::int32_t>(i), utils::generators::GenerateUuid()});
        client->ExecuteDecompose(OperationType::kReadWrite, "INSERT INTO test VALUES (?, ?)", rows_to_insert.back());
    }

    std::vector<Row> cursor_rows;
    cursor_rows.reserve(rows_count);

    client->GetCursor<Row>(OperationType::kReadOnly, 7, "SELECT * FROM test").ForEach([&cursor_rows](Row&& row) {
        cursor_rows.push_back(std::move(row));
    });
    EXPECT_EQ(cursor_rows, rows_to_insert);

    const auto db_rows = client->Execute(OperationType::kReadOnly, "SELECT * FROM test").AsVector<Row>();
    EXPECT_EQ(cursor_rows, db_rows);
}

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
