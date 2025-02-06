#include <userver/utest/utest.hpp>

#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include <userver/storages/sqlite.hpp>
#include <userver/storages/sqlite/exceptions.hpp>
#include <userver/storages/sqlite/execution_result.hpp>
#include <userver/storages/sqlite/result_set.hpp>
#include <userver/storages/sqlite/row_types.hpp>
#include <userver/storages/sqlite/tests/utils.hpp>
#include <userver/utest/assert_macros.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

// Here we want to test the operation of the ResultSet itself (iterator
// invariants, iteration, row access, container conversion and conversion
// container elements into the correct types, including user-defined types).
// All this can be done without being tied to the way we getting the ResultSet

namespace {

struct Row final {
  int id{};
  std::string value;

  bool operator==(const Row& other) const {
    return std::tie(id, value) == std::tie(other.id, other.value);
  }
};

using RowTuple = std::tuple<int, std::string>;

constexpr std::string_view kSelectAllRows = "SELECT * FROM test";
constexpr std::string_view kSelectOneRow = "SELECT * FROM test WHERE id=1";
constexpr std::string_view kSelectNullRow = "SELECT * FROM test WHERE id=NULL";
constexpr std::string_view kSelectAllFields = "SELECT value FROM test";
constexpr std::string_view kSelectOneField =
    "SELECT value FROM test WHERE id=1";
constexpr std::string_view kSelectNullField =
    "SELECT value FROM test WHERE id=NULL";

class SQLiteResultSet : public SQLiteInMemoryInitConnection {
 public:
  void Init(ConnectionPtr connection) {
    connection->Execute("INSERT INTO test VALUES (1, 'first')");
    connection->Execute("INSERT INTO test VALUES (2, 'second')");
  }
};

}  // namespace

UTEST_F(SQLiteResultSet, AsVectorRowTag) {
  ConnectionPtr conn = CreateConnection();
  Init(conn);

  // Get result as vector of tuples
  {
    std::vector<RowTuple> actual;
    UEXPECT_NO_THROW(
        actual = conn->Execute(kSelectAllRows.data()).AsVector<RowTuple>());

    EXPECT_EQ(actual.size(), 2);
    EXPECT_EQ(actual[0], std::make_tuple(1, "first"));
    EXPECT_EQ(actual[1], std::make_tuple(2, "second"));
  }

  // Get result as vector of structures
  {
    std::vector<Row> actual;
    UEXPECT_NO_THROW(actual =
                         conn->Execute(kSelectAllRows.data()).AsVector<Row>());

    EXPECT_EQ(actual.size(), 2);

    Row first_expected{1, "first"};
    EXPECT_EQ(actual[0], first_expected);

    Row second_expected{2, "second"};
    EXPECT_EQ(actual[1], second_expected);
  }

  // Get empty result as vector of rows
  {
    std::vector<Row> actual;
    UEXPECT_NO_THROW(actual =
                         conn->Execute(kSelectNullRow.data()).AsVector<Row>());

    EXPECT_TRUE(actual.empty());
  }
}

UTEST_F(SQLiteResultSet, AsVectorFieldTag) {
  ConnectionPtr conn = CreateConnection();
  Init(conn);

  // Get result as vector of fields
  {
    std::vector<std::string> actual;
    UEXPECT_NO_THROW(actual = conn->Execute(kSelectAllFields.data())
                                  .AsVector<std::string>(kFieldTag));

    EXPECT_EQ(actual.size(), 2);
    EXPECT_EQ(actual[0], "first");
    EXPECT_EQ(actual[1], "second");
  }

  // Get empty result as vector of rows
  {
    std::vector<std::string> actual;
    UEXPECT_NO_THROW(conn->Execute(kSelectNullField.data())
                         .AsVector<std::string>(kFieldTag));
    EXPECT_TRUE(actual.empty());
  }

  // Throw exception if try to get set of row as vector of fields
  {
    UEXPECT_THROW(
        conn->Execute(kSelectAllRows.data()).AsVector<std::string>(kFieldTag),
        SQLiteException);
  }
}

UTEST_F(SQLiteResultSet, AsSingleRow) {
  ConnectionPtr conn = CreateConnection();
  Init(conn);

  // Get result as a single tuple
  {
    RowTuple actual;
    UEXPECT_NO_THROW(
        actual = conn->Execute(kSelectOneRow.data()).AsSingleRow<RowTuple>());

    EXPECT_EQ(actual, std::make_tuple(1, "first"));
  }

  // Get result as a single struct
  {
    Row actual;
    UEXPECT_NO_THROW(
        actual = conn->Execute(kSelectOneRow.data()).AsSingleRow<Row>());

    Row expected{1, "first"};
    EXPECT_EQ(actual, expected);
  }

  // Throw exception if try to get single row from empty result
  {
    UEXPECT_THROW(conn->Execute(kSelectNullRow.data()).AsSingleRow<Row>(),
                  SQLiteException);
  }

  // TODO: What behavior is more preferable here?
  // Throw exception if try to get single row from result with more than one
  // rows
  {
    UEXPECT_THROW(conn->Execute(kSelectAllRows.data()).AsSingleRow<Row>(),
                  SQLiteException);
  }
}

UTEST_F(SQLiteResultSet, AsSingleField) {
  ConnectionPtr conn = CreateConnection();
  Init(conn);

  // Get result as a single field
  {
    std::string actual;
    UEXPECT_NO_THROW(
        actual =
            conn->Execute(kSelectOneField.data()).AsSingleField<std::string>());

    EXPECT_EQ(actual, "first");
  }

  // Throw exception if result is empty
  {
    UEXPECT_THROW(
        conn->Execute(kSelectNullField.data()).AsSingleField<std::string>(),
        SQLiteException);
  }

  // Throw exception if try to get result row as single field
  {
    UEXPECT_THROW(
        conn->Execute(kSelectOneRow.data()).AsSingleField<std::string>(),
        SQLiteException);
  }

  // Throw exception if try to get result with more than one fields as single
  // field
  {
    UEXPECT_THROW(
        conn->Execute(kSelectAllFields.data()).AsSingleField<std::string>(),
        SQLiteException);
  }
}

UTEST_F(SQLiteResultSet, AsOptionalSingleRow) {
  ConnectionPtr conn = CreateConnection();
  Init(conn);

  // Get empty result as an optional single tuple
  {
    std::optional<RowTuple> actual;
    UEXPECT_NO_THROW(actual = conn->Execute(kSelectNullRow.data())
                                  .AsOptionalSingleRow<RowTuple>());

    EXPECT_FALSE(actual.has_value());
  }

  // Get non-empty result as an optional single struct
  {
    std::optional<Row> actual;
    UEXPECT_NO_THROW(
        actual =
            conn->Execute(kSelectOneRow.data()).AsOptionalSingleRow<Row>());

    EXPECT_TRUE(actual.has_value());
    Row expected{1, "first"};
    EXPECT_EQ(actual.value(), expected);
  }

  // TODO: What behavior is more preferable here?
  // Throw exception if try to get result with more than one
  // rows as optional single row
  {
    UEXPECT_THROW(
        conn->Execute(kSelectAllRows.data()).AsOptionalSingleRow<Row>(),
        SQLiteException);
  }
}

UTEST_F(SQLiteResultSet, AsOptionalSingleField) {
  ConnectionPtr conn = CreateConnection();
  Init(conn);

  // Get empty result as a optional single field
  {
    std::optional<std::string> actual;
    UEXPECT_NO_THROW(actual = conn->Execute(kSelectNullField.data())
                                  .AsOptionalSingleField<std::string>());

    EXPECT_FALSE(actual.has_value());
  }

  // Get non-empty result as a optional single field
  {
    std::optional<std::string> actual;
    UEXPECT_NO_THROW(actual = conn->Execute(kSelectOneField.data())
                                  .AsOptionalSingleField<std::string>());

    EXPECT_TRUE(actual.has_value());
    EXPECT_EQ(actual.value(), "first");
  }

  // Throw exception if try to get result with more than one fields as single
  // field
  {
    UEXPECT_THROW(conn->Execute(kSelectAllFields.data())
                      .AsOptionalSingleField<std::string>(),
                  SQLiteException);
  }

  // Throw exception if try to get result row as single field
  {
    UEXPECT_THROW(conn->Execute(kSelectOneRow.data())
                      .AsOptionalSingleField<std::string>(),
                  SQLiteException);
  }
}

UTEST_F(SQLiteResultSet, AsExecutionResult) {
  ConnectionPtr conn = CreateConnection();

  // Get execution result for INSERT query
  {
    ExecutionResult actual;
    UEXPECT_NO_THROW(actual =
                         conn->Execute("INSERT INTO test VALUES (1, 'first')")
                             .AsExecutionResult());
    EXPECT_EQ(actual.rows_affected, 1);
    EXPECT_EQ(actual.last_insert_id, 1);
  }

  // Call execution result on read-only query is safe
  {
    UEXPECT_NO_THROW(conn->Execute(kSelectAllRows.data()).AsExecutionResult());
  }
}

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
