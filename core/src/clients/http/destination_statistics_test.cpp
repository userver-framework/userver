#include <utest/simple_server.hpp>
#include <utest/utest.hpp>

#include <clients/http/client.hpp>

namespace {
constexpr size_t kHttpIoThreads{1};
}

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

TEST(DestinationStatistics, Empty) {
  TestInCoro([] {
    std::shared_ptr<clients::http::Client> client =
        clients::http::Client::Create(kHttpIoThreads);

    auto dest_stats = client->GetDestinationStatistics();
    EXPECT_EQ(0, dest_stats.size());
  });
}

TEST(DestinationStatistics, Ok) {
  TestInCoro([] {
    const testing::SimpleServer http_server{
        [](const HttpRequest& request) { return Callback(200, request); }};
    std::shared_ptr<clients::http::Client> client =
        clients::http::Client::Create(kHttpIoThreads);

    auto url = http_server.GetBaseUrl();

    auto response = client->CreateRequest()
                        ->post(url)
                        ->retry(1)
                        ->timeout(std::chrono::milliseconds(100))
                        ->perform();

    auto dest_stats = client->GetDestinationStatistics();
    ASSERT_EQ(1, dest_stats.size());

    auto it = dest_stats.begin();
    EXPECT_EQ(url, it->first);
    ASSERT_NE(nullptr, it->second);

    using namespace clients::http;
    auto stats = InstanceStatistics(*it->second);
    auto ok = static_cast<size_t>(Statistics::ErrorGroup::kOk);
    EXPECT_EQ(1, stats.error_count[ok]);
    for (size_t i = 0; i < Statistics::kErrorGroupCount; i++) {
      if (i != ok) EXPECT_EQ(0, stats.error_count[i]);
    }
  });
}

TEST(DestinationStatistics, Multiple) {
  TestInCoro([] {
    const testing::SimpleServer http_server{
        [](const HttpRequest& request) { return Callback(200, request); }};
    const testing::SimpleServer http_server2{
        [](const HttpRequest& request) { return Callback(500, request); }};
    std::shared_ptr<clients::http::Client> client =
        clients::http::Client::Create(kHttpIoThreads);

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

    auto dest_stats = client->GetDestinationStatistics();
    ASSERT_EQ(2, dest_stats.size());

    EXPECT_EQ(1, dest_stats.count(url));
    EXPECT_EQ(1, dest_stats.count(url2));

    for (auto& it : dest_stats) {
      ASSERT_NE(nullptr, it.second);

      using namespace clients::http;
      auto stats = InstanceStatistics(*it.second);
      auto ok = static_cast<size_t>(Statistics::ErrorGroup::kOk);
      EXPECT_EQ(1, stats.error_count[ok]);
      for (size_t i = 0; i < Statistics::kErrorGroupCount; i++) {
        if (i != ok)
          EXPECT_EQ(0, stats.error_count[i]) << i << " errors must be zero";
      }
    }
  });
}
