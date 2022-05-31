#include <userver/clients/http/client.hpp>

#include <userver/engine/sleep.hpp>
#include <userver/engine/wait_any.hpp>

#include <userver/utest/http_client.hpp>
#include <userver/utest/simple_server.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr char kTestData[] = "Some test Data";
constexpr unsigned kRepetitions = 200;

using HttpResponse = utest::SimpleServer::Response;
using HttpRequest = utest::SimpleServer::Request;
using HttpCallback = utest::SimpleServer::OnRequest;

std::optional<HttpResponse> Process100(const HttpRequest& request) {
  const bool requires_continue =
      (request.find("Expect: 100-continue") != std::string::npos);

  if (requires_continue || request.empty()) {
    return HttpResponse{
        "HTTP/1.1 100 Continue\r\nContent-Length: "
        "0\r\n\r\n",
        HttpResponse::kWriteAndContinue};
  }

  return std::nullopt;
}

HttpResponse EchoSimpleCallback(const HttpRequest& request) {
  LOG_INFO() << "HTTP Server receive: " << request;

  const auto cont = Process100(request);
  if (cont) {
    return *cont;
  }

  const auto data_pos = request.find("\r\n\r\n");
  std::string payload;
  if (data_pos == std::string::npos) {
    // We have no headers after 100 Continue
    payload = request;
  } else {
    payload = request.substr(data_pos + 4);
  }
  LOG_INFO() << "HTTP Server sending payload: " << payload;

  return {
      "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: "
      "text/html\r\nContent-Length: " +
          std::to_string(payload.size()) + "\r\n\r\n" + payload,
      HttpResponse::kWriteAndClose};
}

HttpResponse SleepCallbackBase(const HttpRequest& request,
                               std::chrono::milliseconds sleep_for) {
  LOG_INFO() << "HTTP Server receive: " << request;

  engine::InterruptibleSleepFor(sleep_for);

  return {
      "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: "
      "4096\r\n\r\n" +
          std::string(4096, '@'),
      HttpResponse::kWriteAndClose};
}

HttpResponse SleepCallback(const HttpRequest& request) {
  return SleepCallbackBase(request, utest::kMaxTestWaitTime);
}

/// [HTTP Client - waitany]
std::size_t ProcessReadyRequests(
    std::vector<clients::http::ResponseFuture>& requests,
    engine::Deadline deadline) {
  std::size_t processed_requests = 0;

  while (auto indx = engine::WaitAnyUntil(deadline, requests)) {
    ++processed_requests;

    std::shared_ptr<clients::http::Response> response = requests[*indx].Get();
    EXPECT_TRUE(response->IsOk());
  }

  return processed_requests;
}
/// [HTTP Client - waitany]

}  // namespace

UTEST(HttpClient, WaitAnySingle) {
  auto http_client_ptr = utest::CreateHttpClient();
  const utest::SimpleServer http_echo_server{&EchoSimpleCallback};

  for (unsigned i = 0; i < kRepetitions; ++i) {
    auto echo_future = http_client_ptr->CreateRequest()
                           ->post(http_echo_server.GetBaseUrl(), kTestData)
                           ->retry(1)
                           ->verify(true)
                           ->http_version(clients::http::HttpVersion::k11)
                           ->timeout(utest::kMaxTestWaitTime)
                           ->async_perform();

    auto resp = engine::WaitAny(echo_future);
    ASSERT_TRUE(resp);
    EXPECT_EQ(*resp, 0);
    auto response = echo_future.Get();
    EXPECT_TRUE(response->IsOk());
    EXPECT_EQ(response->body_view(), kTestData);
  }
}

UTEST(HttpClient, WaitAny) {
  auto http_client_ptr = utest::CreateHttpClient();
  const utest::SimpleServer http_sleep_server{&SleepCallback};
  const utest::SimpleServer http_echo_server{&EchoSimpleCallback};

  for (unsigned i = 0; i < kRepetitions; ++i) {
    auto timeout_future = http_client_ptr->CreateRequest()
                              ->post(http_sleep_server.GetBaseUrl(), kTestData)
                              ->retry(1)
                              ->verify(true)
                              ->http_version(clients::http::HttpVersion::k11)
                              ->timeout(utest::kMaxTestWaitTime)
                              ->async_perform();

    auto echo_future = http_client_ptr->CreateRequest()
                           ->post(http_echo_server.GetBaseUrl(), kTestData)
                           ->retry(1)
                           ->verify(true)
                           ->http_version(clients::http::HttpVersion::k11)
                           ->timeout(utest::kMaxTestWaitTime)
                           ->async_perform();

    auto resp = engine::WaitAny(timeout_future, echo_future);
    ASSERT_TRUE(resp);
    EXPECT_EQ(*resp, 1);
    auto response = echo_future.Get();
    EXPECT_TRUE(response->IsOk());
    EXPECT_EQ(response->body_view(), kTestData);
  }
}

UTEST(HttpClient, WaitAnyMany) {
  auto http_client_ptr = utest::CreateHttpClient();
  const utest::SimpleServer http_echo_server{&EchoSimpleCallback};

  std::vector<clients::http::ResponseFuture> async_requests;
  async_requests.reserve(kRepetitions);

  for (unsigned i = 0; i < kRepetitions; ++i) {
    async_requests.push_back(
        http_client_ptr->CreateRequest()
            ->post(http_echo_server.GetBaseUrl(), kTestData)
            ->retry(1)
            ->verify(true)
            ->http_version(clients::http::HttpVersion::k11)
            ->timeout(utest::kMaxTestWaitTime)
            ->async_perform());
  }

  const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  const std::size_t processed = ProcessReadyRequests(async_requests, deadline);
  EXPECT_EQ(processed, async_requests.size());
  EXPECT_EQ(processed, kRepetitions);
}

USERVER_NAMESPACE_END
