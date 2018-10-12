#include <gtest/gtest.h>

#include <utest/utest.hpp>

#include <server/http/http_request.hpp>

#include "http_request_parser.hpp"

namespace {

const std::string kCrlf = "\r\n";

}  // namespace

using HttpMethod = server::http::HttpMethod;

struct MethodsData {
  std::string method_query;
  HttpMethod orig_method;
  HttpMethod method;
};

class HttpRequestParser : public ::testing::TestWithParam<MethodsData> {};

std::string PrintMethodsDataTestName(
    const ::testing::TestParamInfo<MethodsData>& data) {
  std::string res = data.param.method_query;
  if (res.empty()) res = "_empty_";
  return res;
}

INSTANTIATE_TEST_CASE_P(
    /**/, HttpRequestParser,
    ::testing::Values(
        MethodsData{"DELETE", HttpMethod::kDelete, HttpMethod::kDelete},
        MethodsData{"GET", HttpMethod::kGet, HttpMethod::kGet},
        // HEAD should works as GET except response sending and access logs
        // writing
        MethodsData{"HEAD", HttpMethod::kHead, HttpMethod::kGet},
        MethodsData{"POST", HttpMethod::kPost, HttpMethod::kPost},
        MethodsData{"PUT", HttpMethod::kPut, HttpMethod::kPut},
        MethodsData{"CONNECT", HttpMethod::kConnect, HttpMethod::kConnect},
        MethodsData{"PATCH", HttpMethod::kPatch, HttpMethod::kPatch},
        MethodsData{"GE", HttpMethod::kUnknown, HttpMethod::kUnknown},
        MethodsData{"GETT", HttpMethod::kUnknown, HttpMethod::kUnknown},
        MethodsData{"get", HttpMethod::kUnknown, HttpMethod::kUnknown},
        MethodsData{"", HttpMethod::kUnknown, HttpMethod::kUnknown},
        MethodsData{"XXX", HttpMethod::kUnknown, HttpMethod::kUnknown}),
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
          const auto& http_request_impl =
              dynamic_cast<const server::http::HttpRequestImpl&>(*request);
          const server::http::HttpRequest http_request(http_request_impl);
          EXPECT_EQ(http_request_impl.GetOrigMethod(), param.orig_method);
          EXPECT_EQ(http_request.GetMethod(), param.method);
        });

    const std::string request =
        param.method_query + std::string(" / HTTP/1.1") + kCrlf + kCrlf;

    parser.Parse(request.data(), request.size());

    EXPECT_EQ(parsed, true);
  });
}
