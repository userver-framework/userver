#include <userver/utest/utest.hpp>

#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sqlite3.h>

#include <userver/storages/sqlite.hpp>
#include <userver/storages/sqlite/exceptions.hpp>
#include <userver/storages/sqlite/execution_result.hpp>
#include <userver/storages/sqlite/impl/result_wrapper.hpp>
#include <userver/storages/sqlite/impl/statements_base.hpp>
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

}  // namespace

TEST(ResultSetTest, AsVectorRowTag) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  // Succesful get two rows as vector
  EXPECT_CALL(*mock_sqlite_statement, HasNext())
      .Times(3)
      .WillOnce(::testing::Return(true))
      .WillOnce(::testing::Return(true))
      .WillOnce(::testing::Return(false));
  EXPECT_CALL(*mock_sqlite_statement, Next()).Times(2);
  EXPECT_CALL(*mock_sqlite_statement, GetInt32Column(0))
      .WillOnce(::testing::Return(1))
      .WillOnce(::testing::Return(2));
  EXPECT_CALL(*mock_sqlite_statement, GetStringColumn(1))
      .WillOnce(::testing::Return("first"))
      .WillOnce(::testing::Return("second"));

  ResultSet res(std::make_shared<impl::ResultWrapper>(mock_sqlite_statement));
  std::vector<RowTuple> actual = std::move(res).AsVector<RowTuple>();

  EXPECT_EQ(actual.size(), 2);
  EXPECT_EQ(actual[0], std::make_tuple(1, std::string("first")));
  EXPECT_EQ(actual[1], std::make_tuple(2, std::string("second")));
}

TEST(ResultSetTest, AsVectorRowEmpty) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  // Get empty result as vector
  EXPECT_CALL(*mock_sqlite_statement, HasNext())
      .WillOnce(::testing::Return(false));

  ResultSet res(std::make_shared<impl::ResultWrapper>(mock_sqlite_statement));
  auto actual = std::move(res).AsVector<Row>();

  EXPECT_TRUE(actual.empty());
}

TEST(ResultSetTest, AsVectorFieldTag) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  // Get two rows with one field as vector
  EXPECT_CALL(*mock_sqlite_statement, HasNext())
      .Times(3)
      .WillOnce(::testing::Return(true))
      .WillOnce(::testing::Return(true))
      .WillOnce(::testing::Return(false));
  EXPECT_CALL(*mock_sqlite_statement, Next()).Times(2);
  EXPECT_CALL(*mock_sqlite_statement, ColumnCount())
      .WillRepeatedly(::testing::Return(1));
  EXPECT_CALL(*mock_sqlite_statement, GetStringColumn(0))
      .WillOnce(::testing::Return("first"))
      .WillOnce(::testing::Return("second"));

  ResultSet res(std::make_shared<impl::ResultWrapper>(mock_sqlite_statement));
  auto actual = std::move(res).AsVector<std::string>(kFieldTag);

  EXPECT_EQ(actual.size(), 2);
  EXPECT_EQ(actual[0], "first");
  EXPECT_EQ(actual[1], "second");
}

TEST(ResultSetTest, AsVectorFieldTagThrowsOnMultipleColumns) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, ColumnCount())
      .WillOnce(::testing::Return(2));

  ResultSet res(std::make_shared<impl::ResultWrapper>(mock_sqlite_statement));
  EXPECT_THROW(std::move(res).AsVector<std::string>(kFieldTag),
               SQLiteException);
}

TEST(ResultSetTest, AsSingleRow) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, IsDone())
      .WillOnce(::testing::Return(false));
  EXPECT_CALL(*mock_sqlite_statement, Next()).Times(1);
  EXPECT_CALL(*mock_sqlite_statement, GetInt32Column(0))
      .WillOnce(::testing::Return(1));
  EXPECT_CALL(*mock_sqlite_statement, GetStringColumn(1))
      .WillOnce(::testing::Return("first"));
  EXPECT_CALL(*mock_sqlite_statement, HasNext())
      .WillOnce(::testing::Return(false));

  ResultSet res(std::make_shared<impl::ResultWrapper>(mock_sqlite_statement));
  auto actual = std::move(res).AsSingleRow<Row>();

  EXPECT_EQ(actual, (Row{1, "first"}));
}

TEST(ResultSetTest, AsSingleRowThrowsWhenEmpty) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, IsDone())
      .WillOnce(::testing::Return(true));

  ResultSet res(std::make_shared<impl::ResultWrapper>(mock_sqlite_statement));
  EXPECT_THROW(std::move(res).AsSingleRow<Row>(), SQLiteException);
}

TEST(ResultSetTest, AsSingleRowThrowsWhenMultipleRows) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, IsDone())
      .WillOnce(::testing::Return(false));
  EXPECT_CALL(*mock_sqlite_statement, Next()).Times(1);
  EXPECT_CALL(*mock_sqlite_statement, HasNext())
      .WillOnce(::testing::Return(true));

  ResultSet res(std::make_shared<impl::ResultWrapper>(mock_sqlite_statement));
  EXPECT_THROW(std::move(res).AsSingleRow<Row>(), SQLiteException);
}

TEST(ResultSetTest, AsSingleField) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, IsDone())
      .WillOnce(::testing::Return(false));
  EXPECT_CALL(*mock_sqlite_statement, ColumnCount())
      .WillRepeatedly(::testing::Return(1));
  EXPECT_CALL(*mock_sqlite_statement, GetStringColumn(0))
      .WillOnce(::testing::Return("first"));
  EXPECT_CALL(*mock_sqlite_statement, Next()).Times(1);
  EXPECT_CALL(*mock_sqlite_statement, HasNext())
      .WillOnce(::testing::Return(false));

  ResultSet res(std::make_shared<impl::ResultWrapper>(mock_sqlite_statement));
  auto actual = std::move(res).AsSingleField<std::string>();

  EXPECT_EQ(actual, "first");
}

TEST(ResultSetTest, AsSingleFieldThrowsOnMultipleColumns) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, IsDone())
      .WillOnce(::testing::Return(false));
  EXPECT_CALL(*mock_sqlite_statement, ColumnCount())
      .WillOnce(::testing::Return(2));

  ResultSet res(std::make_shared<impl::ResultWrapper>(mock_sqlite_statement));
  EXPECT_THROW(std::move(res).AsSingleField<std::string>(), SQLiteException);
}

TEST(ResultSetTest, AsOptionalSingleRow) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  // Non-empty case
  EXPECT_CALL(*mock_sqlite_statement, IsDone())
      .WillOnce(::testing::Return(false));
  EXPECT_CALL(*mock_sqlite_statement, Next()).Times(1);
  EXPECT_CALL(*mock_sqlite_statement, GetInt32Column(0))
      .WillOnce(::testing::Return(1));
  EXPECT_CALL(*mock_sqlite_statement, GetStringColumn(1))
      .WillOnce(::testing::Return("first"));
  EXPECT_CALL(*mock_sqlite_statement, HasNext())
      .WillOnce(::testing::Return(false));

  ResultSet res(std::make_shared<impl::ResultWrapper>(mock_sqlite_statement));
  auto actual = std::move(res).AsOptionalSingleRow<Row>();

  EXPECT_TRUE(actual.has_value());
  EXPECT_EQ(actual.value(), (Row{1, "first"}));
}

TEST(ResultSetTest, AsOptionalSingleRowEmpty) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, IsDone())
      .WillRepeatedly(::testing::Return(true));

  ResultSet res(std::make_shared<impl::ResultWrapper>(mock_sqlite_statement));
  auto actual = std::move(res).AsOptionalSingleRow<Row>();

  EXPECT_FALSE(actual.has_value());
}

TEST(ResultSetTest, AsOptionalSingleRowThrowsOnMultipleRows) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, IsDone())
      .WillOnce(::testing::Return(false));
  EXPECT_CALL(*mock_sqlite_statement, Next()).Times(1);
  EXPECT_CALL(*mock_sqlite_statement, HasNext())
      .WillOnce(::testing::Return(true));

  ResultSet res(std::make_shared<impl::ResultWrapper>(mock_sqlite_statement));
  EXPECT_THROW(std::move(res).AsOptionalSingleRow<Row>(), SQLiteException);
}

TEST(ResultSetTest, AsOptionalSingleField) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, IsDone())
      .WillOnce(::testing::Return(false));
  EXPECT_CALL(*mock_sqlite_statement, ColumnCount())
      .WillRepeatedly(::testing::Return(1));
  EXPECT_CALL(*mock_sqlite_statement, Next()).Times(1);
  EXPECT_CALL(*mock_sqlite_statement, GetStringColumn(0))
      .WillOnce(::testing::Return("first"));
  EXPECT_CALL(*mock_sqlite_statement, HasNext())
      .WillOnce(::testing::Return(false));

  ResultSet res(std::make_shared<impl::ResultWrapper>(mock_sqlite_statement));
  auto actual = std::move(res).AsOptionalSingleField<std::string>();

  EXPECT_TRUE(actual.has_value());
  EXPECT_EQ(actual.value(), "first");
}

TEST(ResultSetTest, AsOptionalSingleFieldEmpty) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, IsDone())
      .WillOnce(::testing::Return(true));

  ResultSet res(std::make_shared<impl::ResultWrapper>(mock_sqlite_statement));
  auto actual = std::move(res).AsOptionalSingleField<std::string>();

  EXPECT_FALSE(actual.has_value());
}

TEST(ResultSetTest, AsExecutionResult) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, RowsAffected())
      .WillOnce(::testing::Return(1));
  EXPECT_CALL(*mock_sqlite_statement, LastInsertRowId())
      .WillOnce(::testing::Return(1));

  ResultSet res(std::make_shared<impl::ResultWrapper>(mock_sqlite_statement));
  ExecutionResult exec_result;
  EXPECT_NO_THROW(exec_result = std::move(res).AsExecutionResult());

  EXPECT_EQ(exec_result.rows_affected, 1);
  EXPECT_EQ(exec_result.last_insert_id, 1);
}

TEST(ResultSetTest, AsExecutionResultOnReadOnly) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, RowsAffected())
      .WillOnce(::testing::Return(0));
  EXPECT_CALL(*mock_sqlite_statement, LastInsertRowId())
      .WillOnce(::testing::Return(0));

  ResultSet res(std::make_shared<impl::ResultWrapper>(mock_sqlite_statement));
  EXPECT_NO_THROW(std::move(res).AsExecutionResult());
}

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
