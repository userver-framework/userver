#include <utest/http_client.hpp>
#include <utest/simple_server.hpp>
#include <utest/utest.hpp>

#include <unordered_set>

#include <clients/http/client.hpp>
#include <clients/http/destination_statistics.hpp>

using HttpResponse = testing::SimpleServer::Response;
using HttpRequest = testing::SimpleServer::Request;
using HttpCallback = testing::SimpleServer::OnRequest;

static HttpResponse Callback(int code, const HttpRequest& request) {
  LOG_INFO() << "HTTP Server receive: " << request;

  return {"HTTP/1.1 " + std::to_string(code) +
              " OK\r\nConnection: close\r\rContent-Length: "
              "0\r\n\r\n",
          HttpResponse::kWriteAndClose};
}

UTEST(DestinationStatistics, Empty) {
  auto client = utest::CreateHttpClient();

  EXPECT_EQ(client->GetDestinationStatistics().begin(),
            client->GetDestinationStatistics().end());
}

UTEST(DestinationStatistics, Ok) {
  const testing::SimpleServer http_server{
      [](const HttpRequest& request) { return Callback(200, request); }};
  auto client = utest::CreateHttpClient();

  auto url = http_server.GetBaseUrl();

  auto response = client->CreateRequest()
                      ->post(url)
                      ->retry(1)
                      ->timeout(std::chrono::milliseconds(100))
                      ->perform();

  const auto& dest_stats = client->GetDestinationStatistics();
  size_t size = 0;
  for (const auto& [stat_url, stat_ptr] : dest_stats) {
    ASSERT_EQ(1, ++size);

    EXPECT_EQ(url, stat_url);
    ASSERT_NE(nullptr, stat_ptr);

    using namespace clients::http;
    auto stats = InstanceStatistics(*stat_ptr);
    auto ok = static_cast<size_t>(Statistics::ErrorGroup::kOk);
    EXPECT_EQ(1, stats.error_count[ok]);
    for (size_t i = 0; i < Statistics::kErrorGroupCount; i++) {
      if (i != ok) EXPECT_EQ(0, stats.error_count[i]);
    }
  }
}

UTEST(DestinationStatistics, Multiple) {
  const testing::SimpleServer http_server{
      [](const HttpRequest& request) { return Callback(200, request); }};
  const testing::SimpleServer http_server2{
      [](const HttpRequest& request) { return Callback(500, request); }};
  auto client = utest::CreateHttpClient();

  client->SetDestinationMetricsAutoMaxSize(100);

  auto url = http_server.GetBaseUrl();
  auto url2 = http_server2.GetBaseUrl();

  auto response = client->CreateRequest()
                      ->post(url)
                      ->retry(1)
                      ->timeout(std::chrono::milliseconds(100))
                      ->perform();
  response = client->CreateRequest()
                 ->post(url2)
                 ->retry(1)
                 ->timeout(std::chrono::milliseconds(100))
                 ->perform();

  const auto& dest_stats = client->GetDestinationStatistics();
  size_t size = 0;
  std::unordered_set<std::string> expected_urls{url, url2};
  for (const auto& [stat_url, stat_ptr] : dest_stats) {
    ASSERT_LE(++size, 2);

    EXPECT_EQ(1, expected_urls.erase(stat_url));

    ASSERT_NE(nullptr, stat_ptr);

    using namespace clients::http;
    auto stats = InstanceStatistics(*stat_ptr);
    auto ok = static_cast<size_t>(Statistics::ErrorGroup::kOk);
    EXPECT_EQ(1, stats.error_count[ok]);
    for (size_t i = 0; i < Statistics::kErrorGroupCount; i++) {
      if (i != ok)
        EXPECT_EQ(0, stats.error_count[i]) << i << " errors must be zero";
    }
  }
}
