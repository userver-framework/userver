#include <string_view>

#include <gtest/gtest.h>

#include <userver/http/url.hpp>

USERVER_NAMESPACE_BEGIN

using http::UrlDecode;
using http::UrlEncode;

TEST(UrlEncode, Empty) { EXPECT_EQ("", UrlEncode("")); }

TEST(UrlEncode, Latin) {
  constexpr std::string_view str = "SomeText1234567890";
  EXPECT_EQ(str, UrlEncode(str));
}

TEST(UrlEncode, Special) {
  constexpr std::string_view str = "Text with spaces,?&=";
  EXPECT_EQ("Text%20with%20spaces%2C%3F%26%3D", UrlEncode(str));
}

TEST(UrlDecode, Empty) { EXPECT_EQ("", UrlDecode("")); }

TEST(UrlDecode, Latin) {
  constexpr std::string_view str = "SomeText1234567890";
  EXPECT_EQ(str, UrlDecode(str));
}

TEST(UrlDecode, Special) {
  constexpr std::string_view decoded = "Text with spaces,?&=";
  constexpr std::string_view str = "Text%20with%20spaces%2C%3F%26%3D";
  EXPECT_EQ(decoded, UrlDecode(str));
}

TEST(UrlDecode, Truncated1) {
  constexpr std::string_view str = "%2";
  EXPECT_EQ(str, UrlDecode(str));
}

TEST(UrlDecode, Truncated2) {
  constexpr std::string_view str = "%";
  EXPECT_EQ(str, UrlDecode(str));
}

TEST(UrlDecode, Invalid) {
  constexpr std::string_view str = "%%111";
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

TEST(MakeUrl, Multi) {
  EXPECT_EQ("path?a=b&a=c",
            http::MakeUrl("path", {}, {{"a", "b"}, {"a", "c"}}));
}

TEST(MakeUrl, QueryAndMultiquery) {
  EXPECT_EQ("path?a=b&a=c&k=v",
            http::MakeUrl("path", {{"k", "v"}}, {{"a", "b"}, {"a", "c"}}));
}

TEST(ExtractMetaTypeFromUrl, Empty) {
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

TEST(ExtractPath, Smth) {
  EXPECT_EQ("", http::ExtractPath("http://service.com"));
  EXPECT_EQ("/", http::ExtractPath("http://service.com/"));
  EXPECT_EQ("", http::ExtractPath("service.com"));
  EXPECT_EQ("/", http::ExtractPath("service.com/"));
  EXPECT_EQ("/aaa/bbb", http::ExtractPath("service.com/aaa/bbb"));
  EXPECT_EQ("/aaa/bbb/", http::ExtractPath("service.com/aaa/bbb/"));
}

TEST(ExtractHostname, Smth) {
  EXPECT_EQ("service.com", http::ExtractHostname("http://service.com"));
  EXPECT_EQ("service.com", http::ExtractHostname("http://service.com/"));
  EXPECT_EQ("service.com", http::ExtractHostname("http://service.com/aaa"));
  EXPECT_EQ("service.com", http::ExtractHostname("service.com"));
  EXPECT_EQ("service.com", http::ExtractHostname("service.com/"));
  EXPECT_EQ("service.com", http::ExtractHostname("service.com/aaa/bbb"));

  EXPECT_EQ("service.com", http::ExtractHostname("http://user@service.com"));
  EXPECT_EQ("service.com",
            http::ExtractHostname("http://user:pass@service.com"));

  EXPECT_EQ("service.com", http::ExtractHostname("http://service.com:80"));
  EXPECT_EQ("service.com", http::ExtractHostname("http://service.com:80/"));

  EXPECT_EQ("127.0.0.1", http::ExtractHostname("http://127.0.0.1:80"));
  EXPECT_EQ("127.0.0.1", http::ExtractHostname("http://127.0.0.1"));

  EXPECT_EQ("[::1]", http::ExtractHostname("http://[::1]:80/"));
  EXPECT_EQ("[::1]", http::ExtractHostname("http://[::1]:80"));
  EXPECT_EQ("[::1]", http::ExtractHostname("http://[::1]"));
}

USERVER_NAMESPACE_END
