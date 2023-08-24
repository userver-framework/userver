#include <storages/postgres/tests/util_pgtest.hpp>

#include <userver/storages/postgres/result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;

UTEST_P(PostgreConnection, EmptyResult) {
  CheckConnection(GetConn());

  pg::ResultSet res{nullptr};
  UEXPECT_NO_THROW(res = GetConn()->Execute("select limit 0"));

  ASSERT_EQ(0, res.Size());
  UEXPECT_THROW(res[0], pg::RowIndexOutOfBounds);
  UEXPECT_THROW(res.Front(), pg::RowIndexOutOfBounds);
  UEXPECT_THROW(res.Back(), pg::RowIndexOutOfBounds);
  EXPECT_TRUE(res.begin() == res.end());
  EXPECT_TRUE(res.cbegin() == res.cend());
  EXPECT_TRUE(res.rbegin() == res.rend());
  EXPECT_TRUE(res.crbegin() == res.crend());
}

UTEST_P(PostgreConnection, ResultEmptyRow) {
  CheckConnection(GetConn());

  pg::ResultSet res{nullptr};
  UEXPECT_NO_THROW(res = GetConn()->Execute("select"));

  ASSERT_EQ(1, res.Size());
  UASSERT_NO_THROW(res[0]);
  UEXPECT_NO_THROW(res.Front());
  UEXPECT_NO_THROW(res.Back());
  EXPECT_FALSE(res.begin() == res.end());
  EXPECT_FALSE(res.cbegin() == res.cend());
  EXPECT_FALSE(res.rbegin() == res.rend());
  EXPECT_FALSE(res.crbegin() == res.crend());

  ASSERT_EQ(0, res[0].Size());
  UEXPECT_THROW(res[0][0], pg::FieldIndexOutOfBounds);
  EXPECT_TRUE(res[0].begin() == res[0].end());
  EXPECT_TRUE(res[0].cbegin() == res[0].cend());
  EXPECT_TRUE(res[0].rbegin() == res[0].rend());
  EXPECT_TRUE(res[0].crbegin() == res[0].crend());
}

UTEST_P(PostgreConnection, ResultOobAccess) {
  CheckConnection(GetConn());

  pg::ResultSet res{nullptr};
  UEXPECT_NO_THROW(res = GetConn()->Execute("select 1"));

  ASSERT_EQ(1, res.Size());
  UASSERT_NO_THROW(res[0]);
  UEXPECT_NO_THROW(res.Front());
  UEXPECT_NO_THROW(res.Back());
  EXPECT_FALSE(res.begin() == res.end());
  EXPECT_FALSE(res.cbegin() == res.cend());
  EXPECT_FALSE(res.rbegin() == res.rend());
  EXPECT_FALSE(res.crbegin() == res.crend());
  EXPECT_EQ(++res.begin(), res.end());
  EXPECT_EQ(++res.cbegin(), res.cend());
  EXPECT_EQ(++res.crbegin(), res.crend());
  EXPECT_EQ(++res.rbegin(), res.rend());
  EXPECT_EQ(res.cend() - res.cbegin(), 1);
  EXPECT_EQ(res.crend() - res.crbegin(), 1);

  ASSERT_EQ(1, res[0].Size());
  UEXPECT_NO_THROW(res[0][0]);
  EXPECT_FALSE(res[0].begin() == res[0].end());
  EXPECT_FALSE(res[0].cbegin() == res[0].cend());
  EXPECT_FALSE(res[0].rbegin() == res[0].rend());
  EXPECT_FALSE(res[0].crbegin() == res[0].crend());
  EXPECT_EQ(++res[0].begin(), res[0].end());
  EXPECT_EQ(++res[0].cbegin(), res[0].cend());
  EXPECT_EQ(++res[0].crbegin(), res[0].crend());
  EXPECT_EQ(++res[0].rbegin(), res[0].rend());
  EXPECT_EQ(res[0].cend() - res[0].cbegin(), 1);
  EXPECT_EQ(res[0].crend() - res[0].crbegin(), 1);

  UEXPECT_THROW(res[1], pg::RowIndexOutOfBounds);
  UEXPECT_THROW(res[0][1], pg::FieldIndexOutOfBounds);
}

UTEST_P(PostgreConnection, ResultTraverseForward) {
  CheckConnection(GetConn());

  pg::ResultSet res{nullptr};
  UEXPECT_NO_THROW(res = GetConn()->Execute("select * from "
                                            "(values (1, 2), (3, 4)) as data"));
  ASSERT_EQ(2, res.Size());

  int num = 1;
  for (auto row_it = res.cbegin(); row_it != res.cend(); ++row_it) {
    for (auto col_it = row_it->cbegin(); col_it != row_it->cend(); ++col_it) {
      EXPECT_EQ(col_it->As<int>(), num++);
    }
  }
  EXPECT_EQ(5, num);
}

UTEST_P(PostgreConnection, ResultTraverseBackward) {
  CheckConnection(GetConn());

  pg::ResultSet res{nullptr};
  UEXPECT_NO_THROW(res = GetConn()->Execute("select * from "
                                            "(values (4, 3), (2, 1)) as data"));
  ASSERT_EQ(2, res.Size());

  int num = 1;
  for (auto row_it = res.crbegin(); row_it != res.crend(); ++row_it) {
    for (auto col_it = row_it->crbegin(); col_it != row_it->crend(); ++col_it) {
      EXPECT_EQ(col_it->As<int>(), num++);
    }
  }
  EXPECT_EQ(5, num);
}

USERVER_NAMESPACE_END
