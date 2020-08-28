#include <clients/http/client.hpp>

#include <fmt/format.h>
#include <crypto/certificate.hpp>
#include <crypto/private_key.hpp>
#include <engine/async.hpp>
#include <engine/sleep.hpp>
#include <engine/task/task_context.hpp>
#include <logging/log.hpp>
#include <utils/async.hpp>

#include <utest/http_client.hpp>
#include <utest/simple_server.hpp>
#include <utest/utest.hpp>

namespace {

constexpr char kTestData[] = "Test Data";
constexpr unsigned kRepetitions = 200;

constexpr char kTestHeader[] = "X-Test-Header";
constexpr char kTestHeaderMixedCase[] = "x-TEST-headeR";

constexpr char kResponse200WithHeaderPattern[] =
    "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: 0\r\n{}\r\n\r\n";

// Certifiacte for testing was generated via the following command:
//   `openssl req -x509 -sha256 -nodes -newkey rsa:1024 -keyout priv.key -out
//   cert.crt`
constexpr const char* kPrivateKey =
    R"(-----BEGIN PRIVATE KEY-----
MIICdgIBADANBgkqhkiG9w0BAQEFAASCAmAwggJcAgEAAoGBAKTQ8X1VKk8h83n3
eJNoZbXca1qMlFxs3fAcJJmRV/ceFvd9esM8KOzXCemcKZNVi1tyUPt+LXk/Po1i
3COTlU/+EUHO+ISgImtVdjjcE9+hLiPFINcnID2rWWNJ1pyRjqV26fj6oQMCyAW+
7ZdQivH/XtPGNwlGudsZrvxu44VvAgMBAAECgYA4uPxTkSr1fw7HjCcAPG68zzZX
PIiW4pTjXRwvifkHQGDRHmtQo/TFxiBQOQGKBmfmugoq87r8voptqHdw+wropj4Z
qdekZAWXhm8u7kYRG2Q7ZTEgRwQGCeau0hCQ5j+DU3oTM2HttEv1/CsousJrePqw
0Th/LZMUskPKGBREUQJBANmLCm2zfc9GtuQ3wqzhbpDRh3NilSJOwUK3+WOR/UfW
4Yx7Tpr5ZZr8j9Ah+kWB64p77rffErRrEZjH89jLW+kCQQDB87vemsYCz1alOBcT
xn+e7PlfmH2yGIlcJg2mNyZvVqjEPwh4ubqBHtier2jm6AoVhX9lEM4nOoY0i5f2
H3eXAkEA16asvNjtA7f+/7eDBawn1enP04NLgYn+rSwBTkJfiYKrbn6iCqDmp0Bt
NA8qsRK8szhuCdpaCX4GIKU+xo+5WQJACJ+vwMwc9c8GST5fOE/hKM3coLWFEUAq
C2DdxoA5Q0YVJvSuib+oXUlj1Fp0TaAPorlW2sWOhQwDH579WMI5bQJACCDhAqpU
BP99plWnEh4Z1EtTw3Byikn1h0exRvGtO2rnlRXVRzLnsXBX/pn7xyAHP5jPTDFN
+LfCitjxvZmWsQ==
-----END PRIVATE KEY-----)";

constexpr const char* kCertificate =
    R"(-----BEGIN CERTIFICATE-----
MIICgDCCAemgAwIBAgIJANin/30HHMYLMA0GCSqGSIb3DQEBCwUAMFkxCzAJBgNV
BAYTAlJVMRMwEQYDVQQIDApTb21lLVN0YXRlMSEwHwYDVQQKDBhJbnRlcm5ldCBX
aWRnaXRzIFB0eSBMdGQxEjAQBgNVBAMMCWxvY2FsaG9zdDAeFw0xOTEyMTIwODIx
MjhaFw0zMzA4MjAwODIxMjhaMFkxCzAJBgNVBAYTAlJVMRMwEQYDVQQIDApTb21l
LVN0YXRlMSEwHwYDVQQKDBhJbnRlcm5ldCBXaWRnaXRzIFB0eSBMdGQxEjAQBgNV
BAMMCWxvY2FsaG9zdDCBnzANBgkqhkiG9w0BAQEFAAOBjQAwgYkCgYEApNDxfVUq
TyHzefd4k2hltdxrWoyUXGzd8BwkmZFX9x4W9316wzwo7NcJ6Zwpk1WLW3JQ+34t
eT8+jWLcI5OVT/4RQc74hKAia1V2ONwT36EuI8Ug1ycgPatZY0nWnJGOpXbp+Pqh
AwLIBb7tl1CK8f9e08Y3CUa52xmu/G7jhW8CAwEAAaNQME4wHQYDVR0OBBYEFFmN
gh59kCf1PClm3I30jR9/mQO6MB8GA1UdIwQYMBaAFFmNgh59kCf1PClm3I30jR9/
mQO6MAwGA1UdEwQFMAMBAf8wDQYJKoZIhvcNAQELBQADgYEAAGt9Vo5bM3WHTLza
Jd+x3JbeCAMqz831yCAp2kpssNa0rRNfC3QX3GEKWGMjxgUKpS/9V8tHH/K3jI+K
57DUESC0/NBo4r76JIjMga4i7W7Eh5XD1jnPdfvSGumBIks2UMJV7FaZHwUjr4fP
g3n5Bom64kOrAWOk2xcpd0Pm00o=
-----END CERTIFICATE-----)";

constexpr char kResponse301WithHeaderPattern[] =
    "HTTP/1.1 301 OK\r\nConnection: close\r\nContent-Length: 0\r\n{}\r\n\r\n";

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

static std::optional<HttpResponse> process_100(const HttpRequest& request) {
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

struct Response301WithHeader {
  const std::string location;
  const std::string header;

  HttpResponse operator()(const HttpRequest&) const {
    return {
        fmt::format(kResponse301WithHeaderPattern,
                    "Location: " + location + "\r\n" + header),
        HttpResponse::kWriteAndClose,
    };
  }
};

}  // namespace

TEST(HttpClient, PostEcho) {
  TestInCoro([] {
    const testing::SimpleServer http_server{&echo_callback};
    auto http_client_ptr = utest::CreateHttpClient();

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
      auto http_client_ptr = utest::CreateHttpClient();

      engine::current_task::GetCurrentTaskContext()->RequestCancel(
          engine::TaskCancellationReason::kUserRequest);

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
      auto http_client_ptr = utest::CreateHttpClient();

      const auto request = http_client_ptr->CreateRequest()
                               ->post(http_server.GetBaseUrl(), kTestData)
                               ->timeout(std::chrono::milliseconds(100));

      engine::current_task::GetCurrentTaskContext()->RequestCancel(
          engine::TaskCancellationReason::kUserRequest);

      auto future = request->async_perform();
      EXPECT_THROW(future.Wait(), clients::http::CancelException);
    });

    task.Get();
  });
}

TEST(HttpClient, PostShutdownWithPendingRequest) {
  TestInCoro([] {
    const testing::SimpleServer http_server{&sleep_callback};
    auto http_client_ptr = utest::CreateHttpClient();

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
    auto http_client_ptr = utest::CreateHttpClient();

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
    auto http_client_ptr = utest::CreateHttpClient();

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
    auto http_client_ptr = utest::CreateHttpClient();

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
    auto http_client_ptr = utest::CreateHttpClient();

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
    auto http_client_ptr = utest::CreateHttpClient();

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
    auto http_client_ptr = utest::CreateHttpClient();

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
    const auto http_client = utest::CreateHttpClient();

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
    auto http_client_ptr = utest::CreateHttpClient();

    clients::http::Headers headers;
    headers.emplace(kTestHeader, "test");
    headers.emplace(kTestHeaderMixedCase, "notest");  // should be ignored

    for (unsigned i = 0; i < kRepetitions; ++i) {
      const auto response = http_client_ptr->CreateRequest()
                                ->post(http_server.GetBaseUrl(), kTestData)
                                ->retry(1)
                                ->headers(headers)
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
    auto http_client_ptr = utest::CreateHttpClient();

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
      ASSERT_TRUE(response->headers().count(kTestHeaderMixedCase))
          << "Header value is '" << header_value << "'";
      EXPECT_EQ(response->headers()[kTestHeader], header_data)
          << "Header value is '" << header_value << "'";
      EXPECT_EQ(response->headers()[kTestHeaderMixedCase], header_data)
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
    auto pkey = crypto::PrivateKey::LoadFromString(kPrivateKey, "");
    auto cert = crypto::Certificate::LoadFromString(kCertificate);
    auto http_client_ptr = utest::CreateHttpClient();
    const testing::SimpleServer http_server{echo_callback};
    const auto url = http_server.GetBaseUrl();
    const auto ssl_url =
        http_server.GetBaseUrl(testing::SimpleServer::Schema::kHttps);

    // SSL is slow, setting big timeout to avoid test flapping
    const auto kTimeout = std::chrono::seconds(1);

    // Running twice to make sure that after request without a cert the request
    // with a cert succeeds and do not break other request types.
    for (unsigned i = 0; i < 2; ++i) {
      auto response_future = http_client_ptr->CreateRequest()
                                 ->post(ssl_url)
                                 ->timeout(kTimeout)
                                 ->client_key_cert(pkey, cert)
                                 ->async_perform();

      response_future.Wait();
      EXPECT_THROW(response_future.Get(), std::exception)
          << "SSL is not used by the server but the request with private key "
             "succeeded";

      auto response = http_client_ptr->CreateRequest()
                          ->post(url)
                          ->timeout(kTimeout)
                          ->client_key_cert(pkey, cert)
                          ->perform();

      EXPECT_TRUE(response->IsOk());

      response = http_client_ptr->CreateRequest()
                     ->post(url)
                     ->timeout(kTimeout)
                     ->perform();

      EXPECT_TRUE(response->IsOk());
    }
  });
}

TEST(HttpClient, RedirectHeaders) {
  TestInCoro([] {
    auto http_client_ptr = utest::CreateHttpClient();

    const testing::SimpleServer http_server_final{
        Response200WithHeader{"xxx: good"}};

    const testing::SimpleServer http_server_redirect{
        Response301WithHeader{http_server_final.GetBaseUrl(), "xxx: bad"}};

    const auto response = http_client_ptr->CreateRequest()
                              ->post(http_server_redirect.GetBaseUrl())
                              ->timeout(std::chrono::milliseconds(1000))
                              ->perform();

    EXPECT_TRUE(response->IsOk());
    EXPECT_EQ(response->headers()["xxx"], "good");
    EXPECT_EQ(response->headers()["XXX"], "good");
  });
}
