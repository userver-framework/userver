#include <storages/postgres/tests/util_pgtest.hpp>

#include <userver/storages/postgres/result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;

UTEST_F(PostgreConnection, EmptyResult) {
  CheckConnection(conn);

  pg::ResultSet res{nullptr};
  EXPECT_NO_THROW(res = conn->Execute("select limit 0"));

  ASSERT_EQ(0, res.Size());
  EXPECT_THROW(res[0], pg::RowIndexOutOfBounds);
  EXPECT_THROW(res.Front(), pg::RowIndexOutOfBounds);
  EXPECT_THROW(res.Back(), pg::RowIndexOutOfBounds);
  EXPECT_TRUE(res.begin() == res.end());
  EXPECT_TRUE(res.cbegin() == res.cend());
  EXPECT_TRUE(res.rbegin() == res.rend());
  EXPECT_TRUE(res.crbegin() == res.crend());
}

UTEST_F(PostgreConnection, ResultEmptyRow) {
  CheckConnection(conn);

  pg::ResultSet res{nullptr};
  EXPECT_NO_THROW(res = conn->Execute("select"));

  ASSERT_EQ(1, res.Size());
  ASSERT_NO_THROW(res[0]);
  EXPECT_NO_THROW(res.Front());
  EXPECT_NO_THROW(res.Back());
  EXPECT_FALSE(res.begin() == res.end());
  EXPECT_FALSE(res.cbegin() == res.cend());
  EXPECT_FALSE(res.rbegin() == res.rend());
  EXPECT_FALSE(res.crbegin() == res.crend());

  ASSERT_EQ(0, res[0].Size());
  EXPECT_THROW(res[0][0], pg::FieldIndexOutOfBounds);
  EXPECT_TRUE(res[0].begin() == res[0].end());
  EXPECT_TRUE(res[0].cbegin() == res[0].cend());
  EXPECT_TRUE(res[0].rbegin() == res[0].rend());
  EXPECT_TRUE(res[0].crbegin() == res[0].crend());
}

UTEST_F(PostgreConnection, ResultOobAccess) {
  CheckConnection(conn);

  pg::ResultSet res{nullptr};
  EXPECT_NO_THROW(res = conn->Execute("select 1"));

  ASSERT_EQ(1, res.Size());
  ASSERT_NO_THROW(res[0]);
  EXPECT_NO_THROW(res.Front());
  EXPECT_NO_THROW(res.Back());
  EXPECT_FALSE(res.begin() == res.end());
  EXPECT_FALSE(res.cbegin() == res.cend());
  EXPECT_FALSE(res.rbegin() == res.rend());
  EXPECT_FALSE(res.crbegin() == res.crend());

  ASSERT_EQ(1, res[0].Size());
  EXPECT_NO_THROW(res[0][0]);
  EXPECT_FALSE(res[0].begin() == res[0].end());
  EXPECT_FALSE(res[0].cbegin() == res[0].cend());
  EXPECT_FALSE(res[0].rbegin() == res[0].rend());
  EXPECT_FALSE(res[0].crbegin() == res[0].crend());

  EXPECT_THROW(res[1], pg::RowIndexOutOfBounds);
  EXPECT_THROW(res[0][1], pg::FieldIndexOutOfBounds);
}

USERVER_NAMESPACE_END
