#include <gtest/gtest.h>

#include <fmt/format.h>

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/small_string.hpp>
#include <userver/utils/small_string_serialization.hpp>

USERVER_NAMESPACE_BEGIN

TEST(SmallString, Empty) {
  utils::SmallString<10> str;
  EXPECT_TRUE(str.empty());
  EXPECT_EQ(str.size(), 0);
  EXPECT_THROW(str.at(0), std::out_of_range);
  EXPECT_EQ(std::string_view(str), std::string_view());
}

TEST(SmallString, NonEmpty) {
  utils::SmallString<10> str("abcd");
  EXPECT_FALSE(str.empty());
  EXPECT_EQ(str.size(), 4);
  EXPECT_EQ(str.at(0), 'a');
  EXPECT_EQ(str.at(2), 'c');
  EXPECT_EQ(str.at(3), 'd');

  EXPECT_EQ(str.front(), 'a');
  EXPECT_EQ(str.back(), 'd');

  EXPECT_EQ(std::string_view(str), std::string_view("abcd"));
}

TEST(SmallString, PushBack) {
  utils::SmallString<2> str("a");

  str.push_back('b');
  EXPECT_EQ(str, "ab");
  str.push_back('c');
  EXPECT_EQ(str, "abc");
  str.push_back('d');
  EXPECT_EQ(str, "abcd");

  str.pop_back();
  EXPECT_EQ(str, "abc");
  str.pop_back();
  EXPECT_EQ(str, "ab");
  str.pop_back();
  EXPECT_EQ(str, "a");
}

TEST(SmallString, SizeCapacity) {
  utils::SmallString<10> str("abcd");
  str.resize(3, '1');
  EXPECT_EQ(str, "abc");
  EXPECT_EQ(str.size(), 3);

  str.resize(4, '1');
  EXPECT_EQ(str, "abc1");
  EXPECT_EQ(str.size(), 4);

  str.resize(11, '2');
  EXPECT_EQ(str, "abc12222222");
  EXPECT_EQ(str.size(), 11);
  EXPECT_GE(str.capacity(), 11);

  auto capacity = str.capacity();
  str.reserve(capacity + 1);
  EXPECT_GT(str.capacity(), capacity);
}

TEST(SmallString, Assign) {
  utils::SmallString<10> str("abcd");
  utils::SmallString<10> str2;
  str2 = str;
  EXPECT_EQ(str2, "abcd");

  str = "123456789012345";
  EXPECT_EQ(str, "123456789012345");
}

TEST(SmallString, Log) {
  utils::SmallString<10> str("abcd");
  LOG_INFO() << str;
}

TEST(SmallString, Format) {
  utils::SmallString<3> str1("abcd");
  utils::SmallString<5> str2("1234");
  EXPECT_EQ("abcd1234", fmt::format("{}{}", str1, str2));
}

TEST(SmallString, Iterator) {
  utils::SmallString<3> str("12345");
  for (auto& c : str) c++;
  EXPECT_EQ(str, "23456");
}

TEST(SmallString, ConstsIterator) {
  const utils::SmallString<3> str("12345");
  std::string s;
  for (const auto& c : str) s += c;
  EXPECT_EQ(s, "12345");
}

TEST(SmallString, ParseJson) {
  auto j = formats::json::ValueBuilder{"abba"}.ExtractValue();

  auto str = j.As<utils::SmallString<3>>();
  EXPECT_EQ(str, "abba");

  auto str2 = j.As<utils::SmallString<5>>();
  EXPECT_EQ(str2, "abba");
}

TEST(SmallString, SerializeJson) {
  utils::SmallString<3> str("abcd");
  auto j = formats::json::ValueBuilder{str}.ExtractValue();
  EXPECT_EQ(j.As<std::string>(), str);

  utils::SmallString<5> str2("abcd");
  auto j2 = formats::json::ValueBuilder{str2}.ExtractValue();
  EXPECT_EQ(j2.As<std::string>(), str2);
}

TEST(SmallString, Data) {
  utils::SmallString<3> str("abcd");
  str.data()[2] = 'x';
  EXPECT_EQ(str, "abxd");

  const auto& s = str;
  EXPECT_EQ(std::string_view(s.data(), 4), str);
}

TEST(SmallString, Indexing) {
  utils::SmallString<3> str("abcd");
  str[2] = 'x';
  EXPECT_EQ(str, "abxd");
  char c = str[2];
  EXPECT_EQ(c, 'x');

  const auto& s = str;
  EXPECT_EQ(std::string_view(s.data(), 4), str);
}

USERVER_NAMESPACE_END
