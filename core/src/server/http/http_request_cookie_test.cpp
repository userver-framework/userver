#include <userver/utest/utest.hpp>

#include <unordered_map>

#include <fmt/format.h>

#include <userver/server/http/http_request.hpp>

#include <server/http/http_request_parser.hpp>

struct CookiesData {
  std::string name;
  std::string data;
  std::unordered_map<std::string, std::string> expected;
};

class HttpRequestCookies : public ::testing::TestWithParam<CookiesData> {};

std::string PrintCookiesDataTestName(
    const ::testing::TestParamInfo<CookiesData>& data) {
  std::string res = data.param.name;
  if (res.empty()) res = "_empty_";
  return res;
}

INSTANTIATE_UTEST_SUITE_P(
    /**/, HttpRequestCookies,
    ::testing::Values(CookiesData{"empty", "", {}},
                      CookiesData{"lower_only", "a=b", {{"a", "b"}}},
                      CookiesData{"upper_only", "A=B", {{"A", "B"}}},
                      CookiesData{
                          "mixed", "a=B; A=b", {{"a", "B"}, {"A", "b"}}}),
    PrintCookiesDataTestName);

USERVER_NAMESPACE_BEGIN

UTEST_P(HttpRequestCookies, Test) {
  const auto& param = GetParam();
  bool parsed = false;
  auto parser = server::http::HttpRequestParser::CreateTestParser(
      [&param,
       &parsed](std::shared_ptr<server::request::RequestBase>&& request) {
        parsed = true;
        auto& http_request_impl =
            dynamic_cast<server::http::HttpRequestImpl&>(*request);
        const server::http::HttpRequest http_request(http_request_impl);
        EXPECT_EQ(http_request.CookieCount(), param.expected.size());
        for (const auto& [name, value] : param.expected) {
          EXPECT_TRUE(http_request.HasCookie(name));
          EXPECT_EQ(http_request.GetCookie(name), value);
        }
        size_t names_count = 0;
        for (const auto& name : http_request.GetCookieNames()) {
          ++names_count;
          EXPECT_TRUE(param.expected.find(name) != param.expected.end());
        }
        EXPECT_EQ(names_count, param.expected.size());
      });

  const auto request = "GET / HTTP/1.1\r\nCookie: " + param.data + "\r\n\r\n";
  parser.Parse(request.data(), request.size());
  EXPECT_EQ(parsed, true);
}

USERVER_NAMESPACE_END
