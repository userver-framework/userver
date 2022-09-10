#include <curl-ev/string_list.hpp>

#include <gtest/gtest.h>

#include <vector>

USERVER_NAMESPACE_BEGIN

namespace {

std::vector<std::string> ToVector(const curl::string_list& list) {
  std::vector<std::string> result;
  for (const auto* elem = list.native_handle(); elem; elem = elem->next) {
    result.emplace_back(elem->data);
  }
  return result;
}

}  // namespace

TEST(CurlStringList, Add) {
  curl::string_list list;

  list.add("aaa");
  list.add("bbb");

  std::vector<std::string> expected{"aaa", "bbb"};
  EXPECT_EQ(ToVector(list), expected);
}

TEST(CurlStringList, Clear) {
  curl::string_list list;

  list.add("aaa");
  list.add("bbb");
  EXPECT_FALSE(ToVector(list).empty());

  list.clear();
  EXPECT_TRUE(ToVector(list).empty());
  EXPECT_EQ(list.native_handle(), nullptr);

  list.add("aaa");
  list.add("bbb");
  std::vector<std::string> expected{"aaa", "bbb"};
  EXPECT_EQ(ToVector(list), expected);
}

TEST(CurlStringList, FindIf) {
  curl::string_list list;

  list.add("aaa");
  list.add("bbb");

  EXPECT_TRUE(
      list.FindIf([](std::string_view value) { return value == "aaa"; }));
  EXPECT_TRUE(
      list.FindIf([](std::string_view value) { return value == "bbb"; }));
  EXPECT_FALSE(
      list.FindIf([](std::string_view value) { return value == "ccc"; }));
}

TEST(CurlStringList, ReplaceFirstIf) {
  curl::string_list list;

  list.add("aaa");
  list.add("bab");

  EXPECT_TRUE(list.ReplaceFirstIf(
      [](std::string_view value) {
        return value.size() > 1 && value[1] == 'a';
      },
      "cc"));
  std::vector<std::string> expected1{"cc", "bab"};
  EXPECT_EQ(ToVector(list), expected1);

  EXPECT_FALSE(list.ReplaceFirstIf(
      [](std::string_view value) { return value.size() > 3; }, "zxc"));
  EXPECT_EQ(ToVector(list), expected1);

  EXPECT_TRUE(list.ReplaceFirstIf(
      [](std::string_view value) {
        return value.size() > 1 && value[0] == 'b';
      },
      "dadddd"));
  std::vector<std::string> expected2{"cc", "dadddd"};
  EXPECT_EQ(ToVector(list), expected2);
}

USERVER_NAMESPACE_END
