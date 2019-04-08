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

constexpr char kTestHeader[] = "X-Test-Header";

using HttpResponse = testing::SimpleServer::Response;
using HttpRequest = testing::SimpleServer::Request;
using HttpCallback = testing::SimpleServer::OnRequest;
using HttpPorts = testing::SimpleServer::Ports;

static boost::optional<HttpResponse> process_100(const HttpRequest& request) {
  const bool requires_continue =
      (request.find("Expect: 100-continue") != std::string::npos);

  if (requires_continue || request.empty()) {
    return HttpResponse{
        "HTTP/1.1 100 Continue\r\nContent-Length: "
        "0\r\n\r\n",
        HttpResponse::kWriteAndContinue};
  }

  return boost::none;
}

static HttpResponse echo_callback(const HttpRequest& request) {
  LOG_INFO() << "HTTP Server receive: " << request;

  const auto cont = process_100(request);
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
      "HTTP/1.1 200 OK\r\nConnection: close\r\rContent-Type: "
      "text/html\r\nContent-Length: " +
          std::to_string(payload.size()) + "\r\n\r\n" + payload,
      HttpResponse::kWriteAndClose};
}

static HttpResponse put_validate_callback(const HttpRequest& request) {
  LOG_INFO() << "HTTP Server receive: " << request;

  EXPECT_NE(request.find("PUT"), std::string::npos)
      << "PUT request has no PUT in headers: " << request;

  return {
      "HTTP/1.1 200 OK\r\nConnection: close\r\rContent-Length: "
      "0\r\n\r\n",
      HttpResponse::kWriteAndClose};
}

static HttpResponse sleep_callback(const HttpRequest& request) {
  LOG_INFO() << "HTTP Server receive: " << request;

  engine::InterruptibleSleepFor(kMaxTestWaitTime);

  return {
      "HTTP/1.1 200 OK\r\nConnection: close\r\rContent-Length: "
      "4096\r\n\r\n" +
          std::string(4096, '@'),
      HttpResponse::kWriteAndClose};
}

static HttpResponse huge_data_callback(const HttpRequest& request) {
  LOG_INFO() << "HTTP Server receive: " << request;

  const auto cont = process_100(request);
  if (cont) {
    return *cont;
  }

  return {
      "HTTP/1.1 200 OK\r\nConnection: close\r\rContent-Type: "
      "text/html\r\nContent-Length: "
      "100000\r\n\r\n" +
          std::string(100000, '@'),
      HttpResponse::kWriteAndClose};
}

static HttpResponse header_validate_callback(const HttpRequest& request) {
  LOG_INFO() << "HTTP Server receive: " << request;

  const auto first_pos = request.find(kTestHeader);
  EXPECT_NE(first_pos, std::string::npos)
      << "Failed to find header `" << kTestHeader
      << "` in request: " << request;

  const auto second_pos =
      request.find(kTestHeader, first_pos + sizeof(kTestHeader) - 1);
  EXPECT_EQ(second_pos, std::string::npos)
      << "Header `" << kTestHeader
      << "` exists more than once in request: " << request;

  return {
      "HTTP/1.1 200 OK\r\nConnection: close\r\rContent-Length: "
      "0\r\n\r\n",
      HttpResponse::kWriteAndClose};
}

}  // namespace

TEST(HttpClient, PostEcho) {
  TestInCoro([] {
    const testing::SimpleServer http_server({51234}, &echo_callback);
    std::shared_ptr<clients::http::Client> http_client_ptr =
        clients::http::Client::Create(kHttpIoThreads);

    const auto res = http_client_ptr->CreateRequest()
                         ->post("http://127.0.0.1:51234/", kTestData)
                         ->retry(1)
                         ->verify(true)
                         ->http_version(curl::easy::http_version_1_1)
                         ->timeout(std::chrono::milliseconds(100))
                         ->perform();

    EXPECT_EQ(res->body(), kTestData);
  });
}

TEST(HttpClient, PostShutdownWithPendingRequest) {
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

TEST(HttpClient, PostShutdownWithPendingRequestHuge) {
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

TEST(HttpClient, PutEcho) {
  TestInCoro([] {
    const testing::SimpleServer http_server({51234}, &echo_callback);
    std::shared_ptr<clients::http::Client> http_client_ptr =
        clients::http::Client::Create(kHttpIoThreads);

    const auto res = http_client_ptr->CreateRequest()
                         ->put("http://127.0.0.1:51234/", kTestData)
                         ->retry(1)
                         ->verify(true)
                         ->http_version(curl::easy::http_version_1_1)
                         ->timeout(std::chrono::milliseconds(100))
                         ->perform();

    EXPECT_EQ(res->body(), kTestData);
  });
}

TEST(HttpClient, PutValidateHeader) {
  TestInCoro([] {
    const testing::SimpleServer http_server({51234}, &put_validate_callback);
    std::shared_ptr<clients::http::Client> http_client_ptr =
        clients::http::Client::Create(kHttpIoThreads);

    const auto res = http_client_ptr->CreateRequest()
                         ->put("http://127.0.0.1:51234/", kTestData)
                         ->retry(1)
                         ->verify(true)
                         ->http_version(curl::easy::http_version_1_1)
                         ->timeout(std::chrono::milliseconds(100))
                         ->perform();

    EXPECT_TRUE(res->IsOk());
  });
}

TEST(HttpClient, PutShutdownWithPendingRequest) {
  TestInCoro([] {
    const testing::SimpleServer http_server({51234}, &sleep_callback);
    std::shared_ptr<clients::http::Client> http_client_ptr =
        clients::http::Client::Create(kHttpIoThreads);

    for (unsigned i = 0; i < kRepetitions; ++i)
      http_client_ptr->CreateRequest()
          ->put("http://127.0.0.1:51234/", kTestData)
          ->retry(1)
          ->verify(true)
          ->http_version(curl::easy::http_version_1_1)
          ->timeout(std::chrono::milliseconds(100))
          ->async_perform()
          .Detach();  // Do not do like this in production code!

    http_client_ptr.reset();
  });
}

TEST(HttpClient, PutShutdownWithPendingRequestHuge) {
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
          ->put("http://127.0.0.1:51234/", request)
          ->retry(1)
          ->verify(true)
          ->http_version(curl::easy::http_version_1_1)
          ->timeout(std::chrono::milliseconds(100))
          ->async_perform()
          .Detach();  // Do not do like this in production code!
  });
}

TEST(HttpClient, PutShutdownWithHugeResponse) {
  TestInCoro([] {
    const testing::SimpleServer http_server({51234}, &huge_data_callback);
    const std::shared_ptr<clients::http::Client> http_client_ptr =
        clients::http::Client::Create(kHttpIoThreads);

    for (unsigned i = 0; i < kRepetitions; ++i)
      http_client_ptr->CreateRequest()
          ->put("http://127.0.0.1:51234/", kTestData)
          ->retry(1)
          ->verify(true)
          ->http_version(curl::easy::http_version_1_1)
          ->timeout(std::chrono::milliseconds(100))
          ->async_perform()
          .Detach();  // Do not do like this in production code!
  });
}

TEST(HttpClient, PutPostGetRequestsMix) {
  TestInCoro([] {
    const testing::SimpleServer http_server({51234}, &echo_callback);
    std::shared_ptr<clients::http::Client> http_client_ptr =
        clients::http::Client::Create(kHttpIoThreads);

    for (unsigned i = 0; i < kRepetitions; ++i) {
      const auto res_put = http_client_ptr->CreateRequest()
                               ->put("http://127.0.0.1:51234/", kTestData)
                               ->retry(1)
                               ->verify(true)
                               ->http_version(curl::easy::http_version_1_1)
                               ->timeout(std::chrono::milliseconds(100))
                               ->perform();

      EXPECT_EQ(res_put->body(), kTestData);
    }

    for (unsigned i = 0; i < kRepetitions; ++i) {
      const auto res_post = http_client_ptr->CreateRequest()
                                ->post("http://127.0.0.1:51234/", kTestData)
                                ->retry(1)
                                ->verify(true)
                                ->http_version(curl::easy::http_version_1_1)
                                ->timeout(std::chrono::milliseconds(100))
                                ->perform();

      EXPECT_EQ(res_post->body(), kTestData);
    }

    for (unsigned i = 0; i < kRepetitions; ++i) {
      const auto res_post = http_client_ptr->CreateRequest()
                                ->get("http://127.0.0.1:51234/")
                                ->retry(1)
                                ->verify(true)
                                ->http_version(curl::easy::http_version_1_1)
                                ->timeout(std::chrono::milliseconds(100))
                                ->perform();

      EXPECT_EQ(res_post->body(), "");
    }
  });
}

TEST(HttpClient, Headers) {
  TestInCoro([] {
    const testing::SimpleServer http_server({51234}, &header_validate_callback);
    std::shared_ptr<clients::http::Client> http_client_ptr =
        clients::http::Client::Create(kHttpIoThreads);

    for (unsigned i = 0; i < kRepetitions; ++i) {
      [[maybe_unused]] auto response =
          http_client_ptr->CreateRequest()
              ->post("http://127.0.0.1:51234/", kTestData)
              ->retry(1)
              ->headers(
                  {std::pair<const char*, const char*>{kTestHeader, "test"}})
              ->verify(true)
              ->http_version(curl::easy::http_version_1_1)
              ->timeout(std::chrono::milliseconds(100))
              ->perform();
    }

    http_client_ptr.reset();
  });
}
