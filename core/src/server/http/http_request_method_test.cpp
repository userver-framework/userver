#include <userver/utest/utest.hpp>

#include <userver/server/http/http_request.hpp>

#include <server/http/http_request_parser.hpp>

#include "create_parser_test.hpp"

USERVER_NAMESPACE_BEGIN

namespace {
using HttpMethod = server::http::HttpMethod;

struct MethodsData {
  std::string method_query;
  HttpMethod orig_method;
  HttpMethod method;
};

class HttpRequestMethods : public ::testing::TestWithParam<MethodsData> {};

std::string PrintMethodsDataTestName(
    const ::testing::TestParamInfo<MethodsData>& data) {
  std::string res = data.param.method_query;
  if (res.empty()) res = "_empty_";
  return res;
}

}  // namespace

INSTANTIATE_UTEST_SUITE_P(
    /**/, HttpRequestMethods,
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
        MethodsData{"OPTIONS", HttpMethod::kOptions, HttpMethod::kOptions},
        MethodsData{"GE", HttpMethod::kUnknown, HttpMethod::kUnknown},
        MethodsData{"GETT", HttpMethod::kUnknown, HttpMethod::kUnknown},
        MethodsData{"get", HttpMethod::kUnknown, HttpMethod::kUnknown},
        MethodsData{"", HttpMethod::kUnknown, HttpMethod::kUnknown},
        MethodsData{"XXX", HttpMethod::kUnknown, HttpMethod::kUnknown}),
    PrintMethodsDataTestName);

UTEST_P(HttpRequestMethods, Test) {
  const auto& param = GetParam();
  bool parsed = false;
  auto parser = server::CreateTestParser(
      [&param,
       &parsed](std::shared_ptr<server::request::RequestBase>&& request) {
        parsed = true;
        auto& http_request_impl =
            dynamic_cast<server::http::HttpRequestImpl&>(*request);
        const server::http::HttpRequest http_request(http_request_impl);
        EXPECT_EQ(http_request_impl.GetOrigMethod(), param.orig_method);
        EXPECT_EQ(http_request.GetMethod(), param.method);
      });

  const std::string request = param.method_query + " / HTTP/1.1\r\n\r\n";

  parser.Parse(request.data(), request.size());

  EXPECT_EQ(parsed, true);
}

USERVER_NAMESPACE_END
