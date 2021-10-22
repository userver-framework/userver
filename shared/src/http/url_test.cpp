#include <gtest/gtest.h>

#include <userver/http/url.hpp>

USERVER_NAMESPACE_BEGIN

using http::UrlDecode;
using http::UrlEncode;

TEST(UrlEncode, Empty) { EXPECT_EQ("", UrlEncode("")); }

TEST(UrlEncode, Latin) {
  auto str = "SomeText1234567890";
  EXPECT_EQ(str, UrlEncode(str));
}

TEST(UrlEncode, Special) {
  auto str = "Text with spaces,?&=";
  EXPECT_EQ("Text%20with%20spaces%2C%3F%26%3D", UrlEncode(str));
}

TEST(UrlDecode, Empty) { EXPECT_EQ("", UrlDecode("")); }

TEST(UrlDecode, Latin) {
  auto str = "SomeText1234567890";
  EXPECT_EQ(str, UrlDecode(str));
}

TEST(UrlDecode, Special) {
  auto decoded = "Text with spaces,?&=";
  auto str = "Text%20with%20spaces%2C%3F%26%3D";
  EXPECT_EQ(decoded, UrlDecode(str));
}

TEST(UrlDecode, Truncated1) {
  auto str = "%2";
  EXPECT_EQ(str, UrlDecode(str));
}

TEST(UrlDecode, Truncated2) {
  auto str = "%";
  EXPECT_EQ(str, UrlDecode(str));
}

TEST(UrlDecode, Invalid) {
  auto str = "%%111";
  EXPECT_EQ("Q11", UrlDecode(str));
}

TEST(MakeUrl, InitializerList) {
  EXPECT_EQ("path?a=b&c=d", http::MakeUrl("path", {{"a", "b"}, {"c", "d"}}));
}

TEST(MakeUrl, InitializerList2) {
  std::string value = "12345";
  EXPECT_EQ("path?a=12345&c=d",
            http::MakeUrl("path", {{"a", value}, {"c", "d"}}));
}

TEST(ExtractMetaTypeFromUrl, Emtpy) {
  EXPECT_EQ("", http::ExtractMetaTypeFromUrl(""));
}

TEST(ExtractMetaTypeFromUrl, NoQuery) {
  EXPECT_EQ("ya.ru", http::ExtractMetaTypeFromUrl("ya.ru"));
  EXPECT_EQ("ya.ru/some/path", http::ExtractMetaTypeFromUrl("ya.ru/some/path"));
  EXPECT_EQ("http://ya.ru/some/path",
            http::ExtractMetaTypeFromUrl("http://ya.ru/some/path"));
}

TEST(ExtractMetaTypeFromUrl, WithQuery) {
  EXPECT_EQ("ya.ru", http::ExtractMetaTypeFromUrl("ya.ru?abc=cde"));
  EXPECT_EQ("ya.ru", http::ExtractMetaTypeFromUrl("ya.ru?abc=cde&v=x"));
  EXPECT_EQ("https://ya.ru",
            http::ExtractMetaTypeFromUrl("https://ya.ru?abc=cde&v=x"));
  EXPECT_EQ(
      "https://ya.ru/some/path",
      http::ExtractMetaTypeFromUrl("https://ya.ru/some/path?abc=cde&v=x"));
}

USERVER_NAMESPACE_END
