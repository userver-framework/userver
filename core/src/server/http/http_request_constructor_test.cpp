#include <utest/utest.hpp>

#include <server/http/http_request_constructor.hpp>

using server::http::HttpRequestConstructor;

TEST(HttpRequestConstructor, DecodeUrl) {
  std::string str = "Some+String%20x%30";
  EXPECT_EQ("Some String x0", HttpRequestConstructor::UrlDecode(
                                  str.data(), str.data() + str.size(),
                                  HttpRequestConstructor::DecodeMode::Query));
}

TEST(HttpRequestConstructor, DecodeUrlPlus) {
  std::string str = "Some+String";
  EXPECT_EQ("Some String", HttpRequestConstructor::UrlDecode(
                               str.data(), str.data() + str.size(),
                               HttpRequestConstructor::DecodeMode::Query));
}

TEST(HttpRequestConstructor, DecodeCookie) {
  std::string str = "Some+String%20x%30";
  EXPECT_EQ("Some+String x0", server::http::HttpRequestConstructor::UrlDecode(
                                  str.data(), str.data() + str.size(),
                                  HttpRequestConstructor::DecodeMode::Cookie));
}
