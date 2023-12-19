#include <userver/utest/utest.hpp>

#include <server/http/http_request_constructor.hpp>
#include <userver/http/parser/http_request_parse_args.hpp>

USERVER_NAMESPACE_BEGIN

TEST(HttpRequestConstructor, DecodeUrl) {
  std::string str = "Some+String%20x%30";
  EXPECT_EQ("Some String x0", http::parser::UrlDecode(str));
}

TEST(HttpRequestConstructor, DecodeUrlPlus) {
  std::string str = "Some+String";
  EXPECT_EQ("Some String", http::parser::UrlDecode(str));
}

USERVER_NAMESPACE_END
