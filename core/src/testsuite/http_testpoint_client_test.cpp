#include <userver/testsuite/http_testpoint_client.hpp>

#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/testsuite/testpoint_control.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/http_server_mock.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

utest::HttpServerMock::HttpResponse EchoTestpoint(
    const utest::HttpServerMock::HttpRequest& request) {
  EXPECT_EQ(request.path, "/testpoint");
  EXPECT_EQ(request.method, clients::http::HttpMethod::kPost);
  const auto request_body = formats::json::FromString(request.body);
  EXPECT_TRUE(request_body.HasMember("data"));

  utest::HttpServerMock::HttpResponse response{};
  response.response_status = 200;
  response.body = formats::json::ToString(
      formats::json::MakeObject("data", request_body["data"]));
  return response;
}

}  // namespace

UTEST(HttpTestpointClient, Smoke) {
  utest::HttpServerMock mock_server(&EchoTestpoint);
  const auto testpoint_url = mock_server.GetBaseUrl() + "/testpoint";
  const auto http_client = utest::CreateHttpClient();

  testsuite::TestpointControl testpoint_control;
  testsuite::impl::HttpTestpointClient testpoint_client(
      *http_client, testpoint_url, utest::kMaxTestWaitTime);
  testpoint_control.SetClient(testpoint_client);
  testpoint_control.SetAllEnabled();

  formats::json::Value response;
  TESTPOINT_CALLBACK(
      "name",
      [] { return formats::json::ValueBuilder{"foo"}.ExtractValue(); }(),
      [&](const formats::json::Value& json) { response = json; });
  EXPECT_EQ(response, formats::json::ValueBuilder{"foo"}.ExtractValue());
}

USERVER_NAMESPACE_END
