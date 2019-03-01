#include <clients/http/client.hpp>

#include <engine/async.hpp>
#include <engine/sleep.hpp>
#include <logging/log.hpp>

#include <utest/simple_server.hpp>
#include <utest/utest.hpp>

namespace {

constexpr std::size_t kHttpIoThreads{1};
constexpr char kTestData[] = "Test Data";
constexpr unsigned kRepetitions = 200;

using HttpResponse = testing::SimpleServer::Response;
using HttpRequest = testing::SimpleServer::Request;
using HttpCallback = testing::SimpleServer::OnRequest;
using HttpPorts = testing::SimpleServer::Ports;

static HttpResponse sleep_callback(const HttpRequest& request) {
  LOG_INFO() << "HTTP Server receive: " << request;

  engine::InterruptibleSleepFor(kMaxTestWaitTime);

  return {
      "HTTP/1.1 200 OK\r\nConnection: close\r\rContent-Length: "
      "0\r\n\r\n",
      HttpResponse::kWriteAndClose};
}

}  // namespace

TEST(HttpClient, ShutdownWithPendingRequest) {
  TestInCoro([] {
    const testing::SimpleServer http_server({51234}, &sleep_callback);
    std::shared_ptr<clients::http::Client> http_client_ptr =
        clients::http::Client::Create(kHttpIoThreads);

    for (unsigned i = 0; i < kRepetitions; ++i)
      http_client_ptr->CreateRequest()
          ->post("http://127.0.0.1:51234/", kTestData)
          ->retry(1)
          ->verify(true)
          ->http_version(curl::easy::http_version_1_1)
          ->timeout(std::chrono::milliseconds(100))
          ->async_perform()
          .Detach();  // Do not do like this in production code!

    http_client_ptr.reset();
  });
}

TEST(HttpClient, ShutdownWithPendingRequestHuge) {
  TestInCoro([] {
    const testing::SimpleServer http_server({51234}, &sleep_callback);
    const std::shared_ptr<clients::http::Client> http_client_ptr =
        clients::http::Client::Create(kHttpIoThreads);

    std::string request = kTestData;
    for (unsigned i = 0; i < 20; ++i) {
      request += request;
    }

    for (unsigned i = 0; i < kRepetitions; ++i)
      http_client_ptr->CreateRequest()
          ->post("http://127.0.0.1:51234/", request)
          ->retry(1)
          ->verify(true)
          ->http_version(curl::easy::http_version_1_1)
          ->timeout(std::chrono::milliseconds(100))
          ->async_perform()
          .Detach();  // Do not do like this in production code!
  });
}
