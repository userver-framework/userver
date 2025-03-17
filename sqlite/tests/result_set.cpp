#include <userver/utest/utest.hpp>

#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <userver/storages/sqlite/impl/result_wrapper.hpp>
#include <userver/storages/sqlite/tests/utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

UTEST(SQLiteResultSetTest, AsVectorRowTag) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, Next()).Times(3);
  EXPECT_CALL(*mock_sqlite_statement, HasNext())
      .Times(3)
      .WillOnce(testing::Return(true))
      .WillOnce(testing::Return(true))
      .WillOnce(testing::Return(false));
  EXPECT_CALL(*mock_sqlite_statement,
              Extract(testing::_, testing::A<int32_t&>()))
      .WillOnce(testing::SetArgReferee<1>(1))
      .WillOnce(testing::SetArgReferee<1>(2));

  EXPECT_CALL(*mock_sqlite_statement,
              Extract(testing::_, testing::A<std::string&>()))
      .WillOnce(testing::SetArgReferee<1>(std::string("first")))
      .WillOnce(testing::SetArgReferee<1>(std::string("second")));

  ResultSet res(std::make_unique<impl::ResultWrapper>(
      mock_sqlite_statement, engine::current_task::GetTaskProcessor()));
  auto actual = std::move(res).AsVector<RowTuple>();

  EXPECT_EQ(actual.size(), 2);
  EXPECT_EQ(actual[0], std::make_tuple(1, "first"));
  EXPECT_EQ(actual[1], std::make_tuple(2, "second"));
}

UTEST(SQLiteResultSetTest, AsVectorRowEmpty) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, HasNext())
      .WillOnce(::testing::Return(false));

  ResultSet res(std::make_unique<impl::ResultWrapper>(
      mock_sqlite_statement, engine::current_task::GetTaskProcessor()));
  auto actual = std::move(res).AsVector<Row>();

  EXPECT_TRUE(actual.empty());
}

UTEST(SQLiteResultSetTest, AsVectorFieldTag) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, Next()).Times(3);
  EXPECT_CALL(*mock_sqlite_statement, HasNext())
      .Times(3)
      .WillOnce(testing::Return(true))
      .WillOnce(testing::Return(true))
      .WillOnce(testing::Return(false));
  EXPECT_CALL(*mock_sqlite_statement, ColumnCount())
      .WillRepeatedly(testing::Return(1));
  EXPECT_CALL(*mock_sqlite_statement,
              Extract(testing::_, testing::A<std::string&>()))
      .WillOnce(testing::SetArgReferee<1>("first"))
      .WillOnce(testing::SetArgReferee<1>("second"));

  ResultSet res(std::make_unique<impl::ResultWrapper>(
      mock_sqlite_statement, engine::current_task::GetTaskProcessor()));
  auto actual = std::move(res).AsVector<std::string>(kFieldTag);

  EXPECT_EQ(actual.size(), 2);
  EXPECT_EQ(actual[0], "first");
  EXPECT_EQ(actual[1], "second");
}

UTEST(SQLiteResultSetTest, AsVectorFieldTagThrowsOnMultipleColumns) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, HasNext())
      .WillOnce(testing::Return(true));
  EXPECT_CALL(*mock_sqlite_statement, ColumnCount())
      .WillOnce(testing::Return(2));

  ResultSet res(std::make_unique<impl::ResultWrapper>(
      mock_sqlite_statement, engine::current_task::GetTaskProcessor()));
  EXPECT_THROW(std::move(res).AsVector<std::string>(kFieldTag),
               SQLiteException);
}

UTEST(SQLiteResultSetTest, AsSingleRow) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, Next()).Times(2);
  EXPECT_CALL(*mock_sqlite_statement, HasNext())
      .WillOnce(testing::Return(true))
      .WillOnce(testing::Return(false));
  EXPECT_CALL(*mock_sqlite_statement,
              Extract(testing::_, testing::A<int32_t&>()))
      .WillOnce(testing::SetArgReferee<1>(1));
  EXPECT_CALL(*mock_sqlite_statement,
              Extract(testing::_, testing::A<std::string&>()))
      .WillOnce(testing::SetArgReferee<1>("first"));

  ResultSet res(std::make_unique<impl::ResultWrapper>(
      mock_sqlite_statement, engine::current_task::GetTaskProcessor()));
  auto actual = std::move(res).AsSingleRow<Row>();

  EXPECT_EQ(actual, (Row{1, "first"}));
}

UTEST(SQLiteResultSetTest, AsSingleRowThrowsWhenEmpty) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, HasNext())
      .WillOnce(::testing::Return(false));

  ResultSet res(std::make_unique<impl::ResultWrapper>(
      mock_sqlite_statement, engine::current_task::GetTaskProcessor()));
  EXPECT_THROW(std::move(res).AsSingleRow<Row>(), SQLiteException);
}

UTEST(SQLiteResultSetTest, AsSingleRowThrowsWhenMultipleRows) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, Next()).Times(3);
  EXPECT_CALL(*mock_sqlite_statement, HasNext())
      .WillOnce(::testing::Return(true))
      .WillOnce(::testing::Return(true))
      .WillOnce(::testing::Return(false));

  ResultSet res(std::make_unique<impl::ResultWrapper>(
      mock_sqlite_statement, engine::current_task::GetTaskProcessor()));
  EXPECT_THROW(std::move(res).AsSingleRow<Row>(), SQLiteException);
}

UTEST(SQLiteResultSetTest, AsSingleField) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, Next()).Times(2);
  EXPECT_CALL(*mock_sqlite_statement, HasNext())
      .WillOnce(::testing::Return(true))
      .WillOnce(::testing::Return(false));
  EXPECT_CALL(*mock_sqlite_statement, ColumnCount())
      .WillRepeatedly(::testing::Return(1));
  EXPECT_CALL(*mock_sqlite_statement,
              Extract(testing::_, testing::A<std::string&>()))
      .WillOnce(testing::SetArgReferee<1>(std::string("first")));

  ResultSet res(std::make_unique<impl::ResultWrapper>(
      mock_sqlite_statement, engine::current_task::GetTaskProcessor()));
  auto actual = std::move(res).AsSingleField<std::string>();

  EXPECT_EQ(actual, "first");
}

UTEST(SQLiteResultSetTest, AsSingleFieldThrowsOnMultipleColumns) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, HasNext())
      .WillOnce(::testing::Return(true));
  EXPECT_CALL(*mock_sqlite_statement, ColumnCount())
      .WillOnce(::testing::Return(2));

  ResultSet res(std::make_unique<impl::ResultWrapper>(
      mock_sqlite_statement, engine::current_task::GetTaskProcessor()));
  EXPECT_THROW(std::move(res).AsSingleField<std::string>(), SQLiteException);
}

UTEST(SQLiteResultSetTest, AsOptionalSingleRow) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, Next()).Times(2);
  EXPECT_CALL(*mock_sqlite_statement, HasNext())
      .WillOnce(::testing::Return(true))
      .WillOnce(::testing::Return(false));
  EXPECT_CALL(*mock_sqlite_statement,
              Extract(testing::_, testing::A<int32_t&>()))
      .WillOnce(testing::SetArgReferee<1>(1));
  EXPECT_CALL(*mock_sqlite_statement,
              Extract(testing::_, testing::A<std::string&>()))
      .WillOnce(testing::SetArgReferee<1>(std::string("first")));

  ResultSet res(std::make_unique<impl::ResultWrapper>(
      mock_sqlite_statement, engine::current_task::GetTaskProcessor()));
  auto actual = std::move(res).AsOptionalSingleRow<Row>();

  EXPECT_TRUE(actual.has_value());
  EXPECT_EQ(actual.value(), (Row{1, "first"}));
}

UTEST(SQLiteResultSetTest, AsOptionalSingleRowEmpty) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, IsDone())
      .WillRepeatedly(::testing::Return(true));

  ResultSet res(std::make_unique<impl::ResultWrapper>(
      mock_sqlite_statement, engine::current_task::GetTaskProcessor()));
  auto actual = std::move(res).AsOptionalSingleRow<Row>();

  EXPECT_FALSE(actual.has_value());
}

UTEST(SQLiteResultSetTest, AsOptionalSingleRowThrowsOnMultipleRows) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, Next()).Times(3);
  EXPECT_CALL(*mock_sqlite_statement, HasNext())
      .WillOnce(::testing::Return(true))
      .WillOnce(::testing::Return(true))
      .WillOnce(::testing::Return(false));

  ResultSet res(std::make_unique<impl::ResultWrapper>(
      mock_sqlite_statement, engine::current_task::GetTaskProcessor()));
  EXPECT_THROW(std::move(res).AsOptionalSingleRow<Row>(), SQLiteException);
}

UTEST(SQLiteResultSetTest, AsOptionalSingleField) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, Next()).Times(2);
  EXPECT_CALL(*mock_sqlite_statement, HasNext())
      .WillOnce(::testing::Return(true))
      .WillOnce(::testing::Return(false));
  EXPECT_CALL(*mock_sqlite_statement, ColumnCount())
      .WillRepeatedly(::testing::Return(1));
  EXPECT_CALL(*mock_sqlite_statement,
              Extract(testing::_, testing::A<std::string&>()))
      .WillOnce(testing::SetArgReferee<1>("first"));

  ResultSet res(std::make_unique<impl::ResultWrapper>(
      mock_sqlite_statement, engine::current_task::GetTaskProcessor()));
  auto actual = std::move(res).AsOptionalSingleField<std::string>();

  EXPECT_TRUE(actual.has_value());
  EXPECT_EQ(actual.value(), "first");
}

UTEST(SQLiteResultSetTest, AsOptionalSingleFieldEmpty) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, HasNext())
      .WillOnce(::testing::Return(false));

  ResultSet res(std::make_unique<impl::ResultWrapper>(
      mock_sqlite_statement, engine::current_task::GetTaskProcessor()));
  auto actual = std::move(res).AsOptionalSingleField<std::string>();

  EXPECT_FALSE(actual.has_value());
}

UTEST(SQLiteResultSetTest, AsExecutionResult) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, RowsAffected())
      .WillOnce(::testing::Return(1));
  EXPECT_CALL(*mock_sqlite_statement, LastInsertRowId())
      .WillOnce(::testing::Return(1));

  ResultSet res(std::make_unique<impl::ResultWrapper>(
      mock_sqlite_statement, engine::current_task::GetTaskProcessor()));
  ExecutionResult exec_result;
  EXPECT_NO_THROW(exec_result = std::move(res).AsExecutionResult());

  EXPECT_EQ(exec_result.rows_affected, 1);
  EXPECT_EQ(exec_result.last_insert_id, 1);
}

UTEST(SQLiteResultSetTest, AsExecutionResultOnReadOnly) {
  auto mock_sqlite_statement =
      std::make_shared<::testing::NiceMock<MockSQLiteStatement>>();

  EXPECT_CALL(*mock_sqlite_statement, RowsAffected())
      .WillOnce(::testing::Return(0));
  EXPECT_CALL(*mock_sqlite_statement, LastInsertRowId())
      .WillOnce(::testing::Return(0));

  ResultSet res(std::make_unique<impl::ResultWrapper>(
      mock_sqlite_statement, engine::current_task::GetTaskProcessor()));
  EXPECT_NO_THROW(std::move(res).AsExecutionResult());
}

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
