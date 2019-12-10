#include <clients/http/client.hpp>

#include <fmt/format.h>
#include <crypto/load_key.hpp>
#include <engine/async.hpp>
#include <engine/sleep.hpp>
#include <engine/task/task_context.hpp>
#include <logging/log.hpp>
#include <utils/async.hpp>

#include <utest/simple_server.hpp>
#include <utest/utest.hpp>

namespace {

constexpr std::size_t kHttpIoThreads{1};
constexpr char kTestData[] = "Test Data";
constexpr unsigned kRepetitions = 200;

constexpr char kTestHeader[] = "X-Test-Header";

constexpr char kResponse200WithHeaderPattern[] =
    "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: 0\r\n{}\r\n\r\n";

// Certifiacte for testing was generated via the following command:
//   `openssl req -x509 -sha256 -nodes -newkey rsa:2048 -keyout priv.key -out
//   cert.crt`
constexpr const char* kPrivateKey =
    R"(-----BEGIN PRIVATE KEY-----
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDgDpwuPbexxq9F
01KIFQixBPnE3iU9aClP3XY2cc2nntfNbhByduhTjip96XJMnI6aXvI+6oKYy09Z
03Z8HcWICGbMhfL6IKPE33WPZAdFIolnVl2jnjKv/mGdDnmeB0JxFbjOUoNCUaYC
OF9R6rD0KBw9l037f4DujBaAbbhPXCaPZqY6jbnemwNqYQmIQfyD9I9URiDOrqCe
UBHMbZsXxz5vCOrEkbisW22vjv4UcLVT0ZcE0bubmjy4hu4zVDtyPHig728xfsFg
PgJE+O+uhNPJQ/IUX5o53Ga/+4ZWZZMhKsdpz6T1W4dNQK7CQUkA61xR659yJgJ+
X15gLqJnAgMBAAECggEBAJK5XpN1fSbhCoR6V5Cf3Zo2vO2r380vueYAC9qpedhr
z7xKeGDM92VIMxFTX7NFzqjOxmpnHfC7KxKSxQOQZ3umrNMAYNZlq3lQMGcfRReD
/2D5kMaF4YGY3wl/oirXbC4r4GLUa/pxB3pquhklzI2G+r9mpv2sSJ1uhYnC0DC+
2J/Y3HlcXaHmyHB0RAXkr8N8y8Jb1BdBvNhfCt4smY6q3YkGhAvAvNpNIA16kiWQ
Y39YrUcWMUuNKVV58JxCrL6LbqFx+ejySmATzlBa44bFy+JM8Bgnqx00UWzNK54c
YPVQmOY17m4Q/ojGx3qR7F5PJS0t66//wsgDSnOkSCkCgYEA/XgPmWdWNWnArk8Q
qQ/orbAvw8D4K9C79dLISiVBjC2RcvlWOnFGLDjsVLlO36wdKcBuhoIGG0PFPk8M
VAEOdRxw7bbwlW1W7M3jdxwRMu0R2f4CaASfUzdwQWrWuI0zbdczYVKTJPJCiVnl
JPRlfrEXDNp2iRjDWmGXkYyV72sCgYEA4ktdKAYuy+74zLWka742c1N200BenVRt
9pFcQHkMRDLa+fUlXAtUInbq7EGvpg6MxGHMgpTA09EOV44gUk4S4Bc4nhwzBj9q
Sl3J/ELgKZhZLFc/7ShVkuWgdgtY4DXLc6cqjzDW0RmwxhuNyTQ9TPAivcgMyLia
Ht76Bcx2w/UCgYB/W+5qpFPa7tJUQ4IZkNbXPyog8DtCuNVZBZqCNwoih1sILGS5
ZOVfnxKQ17PcC71zly9yAq9Sz9CyKEIHi6haC/pqV3u3eYMt5Z4f4Uh7EEfiAxHu
djQgOkD7fdV6Uei/jlxQ0I8DB3+LSFItKWg+KnlsifD5nim6pkLkbYGBFQKBgB01
UwnWenXSG4T4sQdDHu4VyNGNjmjKPANGUdz0gtPOqJr4vGC8CZkFNl9WPyC04hB6
+xWjs5vjcPF2I8/bye3osWMfCqr0xnhg0LBhxWM5CdGCVXr76Me0Idj6r/cImoEM
A59F04Rbx4haiBt/RaZHnIRYbOX/hc0URLs43999AoGAOhcqKE6lvwF20ZBYWmBl
A7ydf/0yjGzv0Q9KStLURFaP0nel3xJw1GAOiPspHd4zyDFluyISNufZkmvd07VH
hoqCwno+aCjjIzVkzm9IbKDDl7d8ya1WZHRPqpTTnKMNB+GyqE1QjTdZR2SZcf3+
Hw1o7YbDFFQAQ9uqUWLwAOE=
-----END PRIVATE KEY-----)";

class RequestMethodTestData final {
 public:
  using Request = clients::http::Request;
  using TwoArgsFunction = std::shared_ptr<Request> (Request::*)(
      const std::string& url, std::string data_);
  using OneArgFunction =
      std::shared_ptr<Request> (Request::*)(const std::string& url);

  RequestMethodTestData(const char* method_name, const char* data,
                        TwoArgsFunction func)
      : method_name_(method_name), data_(data), func_two_args_(func) {}

  RequestMethodTestData(const char* method_name, const char* data,
                        OneArgFunction func)
      : method_name_(method_name), data_(data), func_one_arg_(func) {}

  template <class Callback>
  bool PerformRequest(const std::string& url, const Callback& callback,
                      clients::http::Client& client) const {
    *callback.method_name = method_name_;
    *callback.data = data_;

    auto request = client.CreateRequest();
    if (func_two_args_) {
      request = (request.get()->*func_two_args_)(url, data_);
    } else {
      request = (request.get()->*func_one_arg_)(url);
    }

    return request->verify(true)
        ->http_version(curl::easy::http_version_1_1)
        ->timeout(std::chrono::milliseconds(100))
        ->perform()
        ->IsOk();
  }

  const char* GetMethodName() const { return method_name_; }

 private:
  const char* method_name_;
  const char* data_;
  TwoArgsFunction func_two_args_{nullptr};
  OneArgFunction func_one_arg_{nullptr};
};

using HttpResponse = testing::SimpleServer::Response;
using HttpRequest = testing::SimpleServer::Request;
using HttpCallback = testing::SimpleServer::OnRequest;

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
      "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: "
      "text/html\r\nContent-Length: " +
          std::to_string(payload.size()) + "\r\n\r\n" + payload,
      HttpResponse::kWriteAndClose};
}

struct validating_shared_callback {
  const std::shared_ptr<std::string> method_name =
      std::make_shared<std::string>();

  const std::shared_ptr<std::string> data = std::make_shared<std::string>();

  HttpResponse operator()(const HttpRequest& request) const {
    LOG_INFO() << "HTTP Server receive: " << request;

    const auto cont = process_100(request);

    EXPECT_FALSE(!!cont) << "This callback does not work with CONTINUE";

    const auto first_line_end = request.find("\r\n");
    EXPECT_LT(request.find(*method_name), first_line_end)
        << "No '" << *method_name
        << "' in first line of a request: " << request;

    const auto header_end = request.find("\r\n\r\n", first_line_end) + 4;
    EXPECT_EQ(request.substr(header_end), *data)
        << "Request body differ from '" << *data
        << "'. Whole request: " << request;

    return {
        "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: "
        "text/html\r\nContent-Length: " +
            std::to_string(request.size()) + "\r\n\r\n" + request,
        HttpResponse::kWriteAndClose};
  }
};

static HttpResponse put_validate_callback(const HttpRequest& request) {
  LOG_INFO() << "HTTP Server receive: " << request;

  EXPECT_NE(request.find("PUT"), std::string::npos)
      << "PUT request has no PUT in headers: " << request;

  return {
      "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: "
      "0\r\n\r\n",
      HttpResponse::kWriteAndClose};
}

static HttpResponse sleep_callback(const HttpRequest& request) {
  LOG_INFO() << "HTTP Server receive: " << request;

  engine::InterruptibleSleepFor(kMaxTestWaitTime);

  return {
      "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: "
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
      "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: "
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
      "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: "
      "0\r\n\r\n",
      HttpResponse::kWriteAndClose};
}

struct Response200WithHeader {
  const std::string header;

  HttpResponse operator()(const HttpRequest&) const {
    return {
        fmt::format(kResponse200WithHeaderPattern, header),
        HttpResponse::kWriteAndClose,
    };
  }
};

}  // namespace

TEST(HttpClient, PostEcho) {
  TestInCoro([] {
    const testing::SimpleServer http_server{&echo_callback};
    std::shared_ptr<clients::http::Client> http_client_ptr =
        clients::http::Client::Create("", kHttpIoThreads);

    const auto res = http_client_ptr->CreateRequest()
                         ->post(http_server.GetBaseUrl(), kTestData)
                         ->retry(1)
                         ->verify(true)
                         ->http_version(curl::easy::http_version_1_1)
                         ->timeout(std::chrono::milliseconds(100))
                         ->perform();

    EXPECT_EQ(res->body(), kTestData);
  });
}

TEST(HttpClient, CancelPre) {
  TestInCoro([] {
    auto task = utils::Async("test", [] {
      const testing::SimpleServer http_server{&echo_callback};
      std::shared_ptr<clients::http::Client> http_client_ptr =
          clients::http::Client::Create("", kHttpIoThreads);

      engine::current_task::GetCurrentTaskContext()->RequestCancel(
          engine::Task::CancellationReason::kUserRequest);

      EXPECT_THROW(http_client_ptr->CreateRequest(),
                   clients::http::CancelException);
    });

    task.Get();
  });
}

TEST(HttpClient, CancelPost) {
  TestInCoro([] {
    auto task = utils::Async("test", [] {
      const testing::SimpleServer http_server{&echo_callback};
      std::shared_ptr<clients::http::Client> http_client_ptr =
          clients::http::Client::Create("", kHttpIoThreads);

      const auto request = http_client_ptr->CreateRequest()
                               ->post(http_server.GetBaseUrl(), kTestData)
                               ->timeout(std::chrono::milliseconds(100));

      engine::current_task::GetCurrentTaskContext()->RequestCancel(
          engine::Task::CancellationReason::kUserRequest);

      auto future = request->async_perform();
      EXPECT_THROW(future.Wait(), clients::http::CancelException);
    });

    task.Get();
  });
}

TEST(HttpClient, PostShutdownWithPendingRequest) {
  TestInCoro([] {
    const testing::SimpleServer http_server{&sleep_callback};
    std::shared_ptr<clients::http::Client> http_client_ptr =
        clients::http::Client::Create("", kHttpIoThreads);

    for (unsigned i = 0; i < kRepetitions; ++i)
      http_client_ptr->CreateRequest()
          ->post(http_server.GetBaseUrl(), kTestData)
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
    const testing::SimpleServer http_server{&sleep_callback};
    const std::shared_ptr<clients::http::Client> http_client_ptr =
        clients::http::Client::Create("", kHttpIoThreads);

    std::string request = kTestData;
    for (unsigned i = 0; i < 20; ++i) {
      request += request;
    }

    for (unsigned i = 0; i < kRepetitions; ++i)
      http_client_ptr->CreateRequest()
          ->post(http_server.GetBaseUrl(), request)
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
    const testing::SimpleServer http_server{&echo_callback};
    std::shared_ptr<clients::http::Client> http_client_ptr =
        clients::http::Client::Create("", kHttpIoThreads);

    const auto res = http_client_ptr->CreateRequest()
                         ->put(http_server.GetBaseUrl(), kTestData)
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
    const testing::SimpleServer http_server{&put_validate_callback};
    std::shared_ptr<clients::http::Client> http_client_ptr =
        clients::http::Client::Create("", kHttpIoThreads);

    const auto res = http_client_ptr->CreateRequest()
                         ->put(http_server.GetBaseUrl(), kTestData)
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
    const testing::SimpleServer http_server{&sleep_callback};
    std::shared_ptr<clients::http::Client> http_client_ptr =
        clients::http::Client::Create("", kHttpIoThreads);

    for (unsigned i = 0; i < kRepetitions; ++i)
      http_client_ptr->CreateRequest()
          ->put(http_server.GetBaseUrl(), kTestData)
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
    const testing::SimpleServer http_server{&sleep_callback};
    const std::shared_ptr<clients::http::Client> http_client_ptr =
        clients::http::Client::Create("", kHttpIoThreads);

    std::string request = kTestData;
    for (unsigned i = 0; i < 20; ++i) {
      request += request;
    }

    for (unsigned i = 0; i < kRepetitions; ++i)
      http_client_ptr->CreateRequest()
          ->put(http_server.GetBaseUrl(), request)
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
    const testing::SimpleServer http_server{&huge_data_callback};
    const std::shared_ptr<clients::http::Client> http_client_ptr =
        clients::http::Client::Create("", kHttpIoThreads);

    for (unsigned i = 0; i < kRepetitions; ++i)
      http_client_ptr->CreateRequest()
          ->put(http_server.GetBaseUrl(), kTestData)
          ->retry(1)
          ->verify(true)
          ->http_version(curl::easy::http_version_1_1)
          ->timeout(std::chrono::milliseconds(100))
          ->async_perform()
          .Detach();  // Do not do like this in production code!
  });
}

TEST(HttpClient, MethodsMix) {
  TestInCoro([] {
    using clients::http::Request;

    const validating_shared_callback callback{};
    const testing::SimpleServer http_server{callback};
    const auto http_client = clients::http::Client::Create("", kHttpIoThreads);

    const RequestMethodTestData tests[] = {
        {"PUT", kTestData, &Request::put},
        {"POST", kTestData, &Request::post},
        {"GET", "", &Request::get},
        {"HEAD", "", &Request::head},
        {"DELETE", "", &Request::delete_method},
        {"PATCH", kTestData, &Request::patch},
    };

    for (const auto method1 : tests) {
      for (const auto method2 : tests) {
        const bool ok1 = method1.PerformRequest(http_server.GetBaseUrl(),
                                                callback, *http_client);
        EXPECT_TRUE(ok1) << "Failed to perform " << method1.GetMethodName();

        const auto ok2 = method2.PerformRequest(http_server.GetBaseUrl(),
                                                callback, *http_client);
        EXPECT_TRUE(ok2) << "Failed to perform " << method2.GetMethodName()
                         << " after " << method1.GetMethodName();
      }
    }
  });
}

TEST(HttpClient, Headers) {
  TestInCoro([] {
    const testing::SimpleServer http_server{&header_validate_callback};
    std::shared_ptr<clients::http::Client> http_client_ptr =
        clients::http::Client::Create("", kHttpIoThreads);

    for (unsigned i = 0; i < kRepetitions; ++i) {
      const auto response = http_client_ptr->CreateRequest()
                                ->post(http_server.GetBaseUrl(), kTestData)
                                ->retry(1)
                                ->headers({std::pair<const char*, const char*>{
                                    kTestHeader, "test"}})
                                ->verify(true)
                                ->http_version(curl::easy::http_version_1_1)
                                ->timeout(std::chrono::milliseconds(100))
                                ->perform();

      EXPECT_TRUE(response->IsOk());
    }

    http_client_ptr.reset();
  });
}

TEST(HttpClient, HeadersAndWhitespaces) {
  TestInCoro([] {
    auto http_client_ptr = clients::http::Client::Create("", kHttpIoThreads);

    const std::string header_data = kTestData;
    const std::string header_values[] = {
        header_data,
        "     " + header_data,
        "\t \t" + header_data,
        "\t \t" + header_data + "   \t",
        "\t \t" + header_data + "\t ",
        header_data + "   \t",
        header_data + "\t ",
    };

    for (const auto& header_value : header_values) {
      const testing::SimpleServer http_server{
          Response200WithHeader{std::string(kTestHeader) + ':' + header_value}};

      const auto response = http_client_ptr->CreateRequest()
                                ->post(http_server.GetBaseUrl())
                                ->timeout(std::chrono::milliseconds(100))
                                ->perform();

      EXPECT_TRUE(response->IsOk())
          << "Header value is '" << header_value << "'";
      ASSERT_TRUE(response->headers().count(kTestHeader))
          << "Header value is '" << header_value << "'";
      EXPECT_EQ(response->headers()[kTestHeader], header_data)
          << "Header value is '" << header_value << "'";
    }
  });
}

// Make sure that certs are setuped and reset on the end of a request.
//
// Smoke test. Fails on MacOS with Segmentation fault while calling
// Request::RequestImpl::on_certificate_request, probably because CURL library
// was misconfigured and uses wrong version of OpenSSL.
TEST(HttpClient, DISABLED_IN_MAC_OS_TEST_NAME(HttpsWithCert)) {
  TestInCoro([] {
    auto pkey = crypto::LoadPrivateKeyFromString(kPrivateKey, "");
    auto http_client_ptr = clients::http::Client::Create("", kHttpIoThreads);
    const testing::SimpleServer http_server{echo_callback};
    const auto url = http_server.GetBaseUrl();
    const auto ssl_url =
        http_server.GetBaseUrl(testing::SimpleServer::Schema::kHttps);

    // Running twice to make sure that after request without a cert the request
    // with a cert succeeds and do not break other request types.
    for (unsigned i = 0; i < 2; ++i) {
      auto response_future = http_client_ptr->CreateRequest()
                                 ->post(ssl_url)
                                 ->timeout(std::chrono::milliseconds(100))
                                 ->client_cert(pkey)
                                 ->async_perform();

      response_future.Wait();
      EXPECT_THROW(response_future.Get(), std::exception)
          << "SSL is not used by the server but the request succeeded";

      const auto response = http_client_ptr->CreateRequest()
                                ->post(url)
                                ->timeout(std::chrono::milliseconds(100))
                                ->client_cert(pkey)
                                ->perform();

      EXPECT_TRUE(response->IsOk());

      const auto response2 = http_client_ptr->CreateRequest()
                                 ->post(url)
                                 ->timeout(std::chrono::milliseconds(100))
                                 ->perform();  // No client cert

      EXPECT_TRUE(response2->IsOk());
    }
  });
}
