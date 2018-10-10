#include <gtest/gtest.h>

#include <utest/utest.hpp>

#include <server/http/http_request.hpp>

#include "http_request_parser.hpp"

namespace {

const std::string kCrlf = "\r\n";

}  // namespace

using MethodsData = std::pair<const char*, server::http::HttpMethod>;

class HttpRequestParser : public ::testing::TestWithParam<MethodsData> {};

std::string PrintMethodsDataTestName(
    const ::testing::TestParamInfo<MethodsData>& data) {
  std::string res = data.param.first;
  if (res.empty()) res = "_empty_";
  return res;
}

INSTANTIATE_TEST_CASE_P(
    /**/, HttpRequestParser,
    ::testing::Values(
        std::make_pair("DELETE", server::http::HttpMethod::kDelete),
        std::make_pair("GET", server::http::HttpMethod::kGet),
        std::make_pair("HEAD", server::http::HttpMethod::kHead),
        std::make_pair("POST", server::http::HttpMethod::kPost),
        std::make_pair("PUT", server::http::HttpMethod::kPut),
        std::make_pair("CONNECT", server::http::HttpMethod::kConnect),
        std::make_pair("PATCH", server::http::HttpMethod::kPatch),
        std::make_pair("GE", server::http::HttpMethod::kUnknown),
        std::make_pair("GETT", server::http::HttpMethod::kUnknown),
        std::make_pair("get", server::http::HttpMethod::kUnknown),
        std::make_pair("", server::http::HttpMethod::kUnknown),
        std::make_pair("XXX", server::http::HttpMethod::kUnknown)),
    PrintMethodsDataTestName);

TEST_P(HttpRequestParser, Methods) {
  TestInCoro([this]() {
    server::http::HandlerInfoIndex handler_info_index;
    server::request::RequestConfig request_config({}, "<test_config>", {});
    const auto& param = GetParam();
    bool parsed = false;
    server::http::HttpRequestParser parser(
        handler_info_index, request_config,
        [&param,
         &parsed](std::unique_ptr<server::request::RequestBase>&& request) {
          parsed = true;
          const server::http::HttpRequest http_request(
              dynamic_cast<const server::http::HttpRequestImpl&>(*request));
          EXPECT_EQ(http_request.GetMethod(), param.second);
        });

    const std::string request =
        param.first + std::string(" / HTTP/1.1") + kCrlf + kCrlf;

    parser.Parse(request.data(), request.size());

    EXPECT_EQ(parsed, true);
  });
}
