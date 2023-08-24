#include <userver/utest/utest.hpp>

#include <userver/clients/http/client.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/logging/log.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/http_server_mock.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
const std::string kRequestBody = "some body";
const std::string kResponseBody = "returned body";
}  // namespace

UTEST(HttpServerMock, Ctr) {
  utest::HttpServerMock mock(
      [](const utest::HttpServerMock::HttpRequest& request) {
        EXPECT_EQ(clients::http::HttpMethod::kPost, request.method);
        EXPECT_EQ("/", request.path);
        EXPECT_EQ("value1", request.headers.at(std::string_view{"a"}));
        EXPECT_EQ("value2", request.headers.at(std::string_view{"header"}));

        EXPECT_EQ((std::unordered_map<std::string, std::string>{
                      {"arg1", "val1"}, {"arg2", "val2"}}),
                  request.query);
        EXPECT_EQ(kRequestBody, request.body);
        return utest::HttpServerMock::HttpResponse{
            287,
            {std::make_pair(std::string{"x"}, std::string{"y"})},
            kResponseBody,
        };
      });

  auto http_client_ptr = utest::CreateHttpClient();
  clients::http::Headers headers{
      std::make_pair(std::string{"a"}, std::string{"value1"}),
      std::make_pair(std::string{"header"}, std::string{"value2"}),
  };
  for (int i = 0; i < 2; i++) {
    auto response =
        http_client_ptr->CreateRequest()
            .post(mock.GetBaseUrl() + "?arg1=val1&arg2=val2", kRequestBody)
            .headers(headers)
            .retry(1)
            .timeout(utest::kMaxTestWaitTime)
            .perform();

    EXPECT_EQ(287, response->status_code());
    EXPECT_EQ(kResponseBody, response->body());
    EXPECT_EQ(
        (clients::http::Headers{
            std::make_pair(std::string{"x"}, std::string{"y"}),
            std::make_pair(std::string{"Content-Length"}, std::string{"13"})}),
        response->headers());
  }
}

USERVER_NAMESPACE_END
