#include <userver/utest/utest.hpp>

#include <server/http/http_request_constructor.hpp>
#include "http_request_parse_args.hpp"

using server::http::HttpRequestConstructor;
using namespace server::http::parser;

TEST(HttpRequestConstructor, DecodeUrl) {
  std::string str = "Some+String%20x%30";
  EXPECT_EQ("Some String x0", UrlDecode(str));
}

TEST(HttpRequestConstructor, DecodeUrlPlus) {
  std::string str = "Some+String";
  EXPECT_EQ("Some String", UrlDecode(str));
}
