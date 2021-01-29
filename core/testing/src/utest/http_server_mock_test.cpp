#include <utest/utest.hpp>

#include <clients/http/client.hpp>
#include <engine/task/task.hpp>
#include <logging/log.hpp>
#include <utest/http_client.hpp>
#include <utest/http_server_mock.hpp>

namespace {
const std::string kRequestBody = "some body";
const std::string kResponseBody = "returned body";
}  // namespace

TEST(HttpServerMock, Ctr) {
  RunInCoro([] {
    testing::HttpServerMock mock(
        [](const testing::HttpServerMock::HttpRequest& request) {
          EXPECT_EQ(clients::http::HttpMethod::kPost, request.method);
          EXPECT_EQ("/", request.path);
          EXPECT_EQ("value1", request.headers.at("a"));
          EXPECT_EQ("value2", request.headers.at("header"));

          EXPECT_EQ((std::unordered_map<std::string, std::string>{
                        {"arg1", "val1"}, {"arg2", "val2"}}),
                    request.query);
          EXPECT_EQ(kRequestBody, request.body);
          return testing::HttpServerMock::HttpResponse{
              287,
              {{"x", "y"}},
              kResponseBody,
          };
        });

    auto http_client_ptr = utest::CreateHttpClient();
    clients::http::Headers headers{
        {"a", "value1"},
        {"header", "value2"},
    };
    for (int i = 0; i < 2; i++) {
      auto response =
          http_client_ptr->CreateRequest()
              ->post(mock.GetBaseUrl() + "?arg1=val1&arg2=val2", kRequestBody)
              ->headers(headers)
              ->retry(1)
              ->timeout(std::chrono::seconds(10))
              ->perform();

      EXPECT_EQ(287, response->status_code());
      EXPECT_EQ(kResponseBody, response->body());
      EXPECT_EQ((clients::http::Headers{{"x", "y"}, {"Content-Length", "13"}}),
                response->headers());
    }
  });
}
