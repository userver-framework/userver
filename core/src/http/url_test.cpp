#include <utest/utest.hpp>

#include <http/url.hpp>

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
