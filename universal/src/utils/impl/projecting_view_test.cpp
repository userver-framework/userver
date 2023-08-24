#include <userver/utils/impl/projecting_view.hpp>

#include <map>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(ProjectingView, Keys) {
  std::map<int, char> cont{
      {1, '1'},
      {2, '2'},
  };
  auto proj = utils::impl::MakeKeysView(cont);
  EXPECT_EQ(*proj.begin(), 1);
  EXPECT_EQ(*++proj.begin(), 2);

  EXPECT_EQ(proj.begin(), proj.begin());
  EXPECT_NE(proj.begin(), proj.end());
  EXPECT_NE(proj.cbegin(), proj.cend());

  auto it = proj.begin();
  ++it;
  ++it;
  EXPECT_EQ(it, proj.end());

  for (const auto& value : proj) {
    EXPECT_TRUE(value == 1 || value == 2);
  }
}

TEST(ProjectingView, Values) {
  std::map<int, char> cont{
      {1, '1'},
      {2, '2'},
  };
  auto proj = utils::impl::MakeValuesView(cont);
  EXPECT_EQ(*proj.begin(), '1');
  EXPECT_EQ(*++proj.begin(), '2');

  *proj.begin() = 'Y';
  EXPECT_EQ(*proj.begin(), 'Y');
  EXPECT_EQ(cont[1], 'Y');

  EXPECT_EQ(proj.begin(), proj.begin());
  EXPECT_NE(proj.begin(), proj.end());
  EXPECT_NE(proj.cbegin(), proj.cend());

  auto it = proj.begin();
  ++it;
  ++it;
  EXPECT_EQ(it, proj.end());
}

TEST(ProjectingView, LambdaValues) {
  std::map<int, char> cont{
      {1, '1'},
      {2, '2'},
  };
  auto proj = utils::impl::ProjectingView(
      cont, [](std::pair<const int, char>& v) -> char& { return v.second; });
  EXPECT_EQ(*proj.begin(), '1');
  EXPECT_EQ(*++proj.begin(), '2');

  *proj.begin() = 'Y';
  EXPECT_EQ(*proj.begin(), 'Y');
  EXPECT_EQ(cont[1], 'Y');

  EXPECT_EQ(proj.begin(), proj.begin());
  EXPECT_NE(proj.begin(), proj.end());
  EXPECT_NE(proj.cbegin(), proj.cend());

  auto it = proj.begin();
  ++it;
  ++it;
  EXPECT_EQ(it, proj.end());
}

USERVER_NAMESPACE_END
