#include <userver/clients/http/client.hpp>

#include <set>

#include <fmt/format.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <clients/http/client_utils_test.hpp>
#include <clients/http/testsuite.hpp>
#include <engine/task/task_processor.hpp>
#include <userver/clients/dns/resolver.hpp>
#include <userver/clients/http/connect_to.hpp>
#include <userver/clients/http/request_tracing_editor.hpp>
#include <userver/clients/http/streamed_response.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/crypto/certificate.hpp>
#include <userver/crypto/private_key.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/tracing.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/userver_info.hpp>

#include <userver/utest/http_client.hpp>
#include <userver/utest/simple_server.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

// For use in tests where we don't expect the timeout to expire.
constexpr auto kTimeout = utest::kMaxTestWaitTime;
// For use in tests where we expect the timeout to expire. It's OK if the
// timeout sometimes expires just because the server was not fast enough to
// process the request (test may very rarely pass as a false positive).
constexpr auto kSmallTimeout = std::chrono::milliseconds{50};

constexpr char kTestData[] = "Test Data";
constexpr unsigned kRepetitions = 200;
constexpr unsigned kFewRepetitions = 8;

constexpr std::string_view kTestHeader = "X-Test-Header";
constexpr std::string_view kTestHeaderMixedCase = "x-TEST-headeR";

constexpr char kTestUserAgent[] = "correct/2.0 (user agent) taxi_userver/000f";

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
  using TwoArgsFunction = std::function<Request&(
      Request&, const std::string& url, std::string data)>;
  using OneArgFunction =
      std::function<Request&(Request&, const std::string& url)>;

  RequestMethodTestData(const char* method_name, const char* data,
                        TwoArgsFunction func)
      : method_name_(method_name), data_(data), func_two_args_(func) {}

  RequestMethodTestData(const char* method_name, const char* data,
                        OneArgFunction func)
      : method_name_(method_name), data_(data), func_one_arg_(func) {}

  template <class Callback>
  auto PrepareRequest(const std::string& url, const Callback& callback,
                      clients::http::Request request) const {
    *callback.method_name = method_name_;
    *callback.data = data_;

    if (func_two_args_) {
      func_two_args_(request, url, data_);
    } else {
      func_one_arg_(request, url);
    }

    return request.verify(true)
        .http_version(clients::http::HttpVersion::k11)
        .timeout(kTimeout);
  }

  const char* GetMethodName() const { return method_name_; }

 private:
  const char* method_name_;
  const char* data_;
  TwoArgsFunction func_two_args_{nullptr};
  OneArgFunction func_one_arg_{nullptr};
};

using HttpResponse = utest::SimpleServer::Response;
using HttpRequest = utest::SimpleServer::Request;
using HttpCallback = utest::SimpleServer::OnRequest;

std::optional<HttpResponse> process_100(const HttpRequest& request) {
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

struct EchoCallback {
  std::shared_ptr<std::size_t> responses_200 = std::make_shared<std::size_t>(0);

  HttpResponse operator()(const HttpRequest& request) const {
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

    ++*responses_200;

    return {
        "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: "
        "text/html\r\nContent-Length: " +
            std::to_string(payload.size()) + "\r\n\r\n" + payload,
        HttpResponse::kWriteAndClose};
  }
};

struct ValidatingSharedCallback {
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

HttpResponse put_validate_callback(const HttpRequest& request) {
  LOG_INFO() << "HTTP Server receive: " << request;

  EXPECT_NE(request.find("PUT"), std::string::npos)
      << "PUT request has no PUT in headers: " << request;

  return {
      "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: "
      "0\r\n\r\n",
      HttpResponse::kWriteAndClose};
}

HttpResponse sleep_callback_base(const HttpRequest& request,
                                 std::chrono::milliseconds sleep_for) {
  LOG_INFO() << "HTTP Server receive: " << request;

  engine::InterruptibleSleepFor(sleep_for);

  return {
      "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: "
      "4096\r\n\r\n" +
          std::string(4096, '@'),
      HttpResponse::kWriteAndClose};
}

HttpResponse sleep_callback(const HttpRequest& request) {
  return sleep_callback_base(request, utest::kMaxTestWaitTime);
}

HttpResponse sleep_callback_1s(const HttpRequest& request) {
  return sleep_callback_base(request, std::chrono::seconds(1));
}

HttpResponse huge_data_callback(const HttpRequest& request) {
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

std::string TryGetHeader(const HttpRequest& request, std::string_view header) {
  const auto first_pos = request.find(header);
  if (first_pos == std::string::npos) return {};
  const auto second_pos = request.find(header, first_pos + header.length());
  EXPECT_EQ(second_pos, std::string::npos)
      << "Header `" << header
      << "` exists more than once in request: " << request;

  auto values_begin_pos = request.find(':', first_pos + header.length()) + 1;
  auto values_end_pos = request.find('\r', values_begin_pos);

  std::string header_value(request.data() + values_begin_pos,
                           values_end_pos - values_begin_pos);
  boost::trim(header_value);
  return header_value;
}

std::string AssertHeader(const HttpRequest& request, std::string_view header) {
  const auto first_pos = request.find(header);
  EXPECT_NE(first_pos, std::string::npos)
      << "Failed to find header `" << header << "` in request: " << request;

  return TryGetHeader(request, header);
}

HttpResponse header_validate_callback(const HttpRequest& request) {
  LOG_INFO() << "HTTP Server receive: " << request;
  AssertHeader(request, kTestHeader);
  return {
      "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: "
      "0\r\n\r\n",
      HttpResponse::kWriteAndClose};
}

HttpResponse user_agent_validate_callback(const HttpRequest& request) {
  LOG_INFO() << "HTTP Server receive: " << request;
  auto header_value = AssertHeader(request, http::headers::kUserAgent);

  EXPECT_EQ(header_value, kTestUserAgent) << "In request: " << request;

  return {
      "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: "
      "0\r\n\r\n",
      HttpResponse::kWriteAndClose};
}

HttpResponse no_user_agent_validate_callback(const HttpRequest& request) {
  LOG_INFO() << "HTTP Server receive: " << request;
  auto header_value = TryGetHeader(request, http::headers::kUserAgent);
  EXPECT_EQ(header_value, utils::GetUserverIdentifier())
      << "In request: " << request;

  return {
      "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: "
      "0\r\n\r\n",
      HttpResponse::kWriteAndClose};
}

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

struct Response503WithConnDrop {
  HttpResponse operator()(const HttpRequest&) const {
    return {
        "HTTP/1.1 503 Service Unavailable\r\n"
        "Content-Length: 0\r\n"
        "\r\n ",
        HttpResponse::kWriteAndClose};
  }
};

struct CheckCookie {
  const std::set<std::string> expected_cookies;

  HttpResponse operator()(const HttpRequest& request) {
    const auto header_pos = request.find("Cookie:");
    EXPECT_NE(header_pos, std::string::npos)
        << "Failed to find 'Cookie' header in request: " << request;

    EXPECT_EQ(request.find("Cookie:", header_pos + 1), std::string::npos)
        << "Duplicate 'Cookie' header in request: " << request;

    const auto value_start = request.find_first_not_of(' ', header_pos + 7);
    EXPECT_NE(value_start, std::string::npos)
        << "Malformed request: " << request;
    const auto value_end = request.find("\r\n", value_start);
    EXPECT_NE(value_end, std::string::npos) << "Malformed request: " << request;

    auto value = request.substr(value_start, value_end - value_start);
    std::vector<std::string> received_cookies;
    boost::split(received_cookies, value, [](char c) { return c == ';'; });

    auto unseen_cookies = expected_cookies;
    for (auto cookie : received_cookies) {
      boost::trim(cookie);
      EXPECT_TRUE(unseen_cookies.erase(cookie))
          << "Unexpected cookie '" << cookie << "' in request: " << request;
    }
    EXPECT_TRUE(unseen_cookies.empty()) << "Not all cookies received";

    return {"HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: 0\r\n\r\n",
            HttpResponse::kWriteAndClose};
  }
};

constexpr auto kTestHosts = R"(
127.0.0.2 localhost
::1 localhost
127.0.0.1 localhost
::2 localhost
)";

struct ResolverWrapper {
  ResolverWrapper()
      : hosts_file{[] {
          auto file = fs::blocking::TempFile::Create();
          fs::blocking::RewriteFileContents(file.GetPath(), kTestHosts);
          return file;
        }()},
        fs_task_processor{
            [] {
              engine::TaskProcessorConfig config;
              config.name = "fs-task-processor";
              config.worker_threads = 1;
              config.thread_name = "fs-worker";
              return config;
            }(),
            engine::current_task::GetTaskProcessor().GetTaskProcessorPools()},
        resolver{fs_task_processor, [=] {
                   clients::dns::ResolverConfig config;
                   config.file_path = hosts_file.GetPath();
                   config.file_update_interval = utest::kMaxTestWaitTime;
                   config.network_timeout = utest::kMaxTestWaitTime;
                   config.network_attempts = 1;
                   config.cache_max_reply_ttl = std::chrono::seconds{1};
                   config.cache_failure_ttl = std::chrono::seconds{1},
                   config.cache_ways = 1;
                   config.cache_size_per_way = 1;
                   return config;
                 }()} {}

  fs::blocking::TempFile hosts_file;
  engine::TaskProcessor fs_task_processor;
  clients::dns::Resolver resolver;
};

auto LogLevelScope(logging::Level level) {
  return logging::DefaultLoggerLevelScope{level};
}

namespace sample {

/// [HTTP Client - request reuse]
std::string DifferentUrlsRetry(std::string data, clients::http::Client& http,
                               std::chrono::milliseconds timeout,
                               std::initializer_list<std::string> urls_list) {
  auto request = http.CreateRequest()
                     .post()
                     .data(std::move(data))  // no copying
                     .retry(1)
                     .http_version(clients::http::HttpVersion::k11)
                     .timeout(timeout);

  for (const auto& url : urls_list) {
    request.url(url);  // set URL

    try {
      auto res = request.perform();
      if (res->IsOk()) {
        return std::move(*res).body();  // no copying
      }
    } catch (const clients::http::TimeoutException&) {
    }
  }

  throw std::runtime_error("No alive servers");
}
/// [HTTP Client - request reuse]

std::string DifferentUrlsRetryStreamResponseBody(
    std::string data, clients::http::Client& http,
    std::chrono::milliseconds timeout,
    std::initializer_list<std::string> urls_list) {
  auto request = http.CreateRequest()
                     .post()
                     .data(std::move(data))  // no copying
                     .retry(1)
                     .http_version(clients::http::HttpVersion::k11)
                     .timeout(timeout);

  for (const auto& url : urls_list) {
    request.url(url);  // set URL
    auto queue = concurrent::StringStreamQueue::Create();

    try {
      auto stream_response = request.async_perform_stream_body(queue);
      const auto status_code = stream_response.StatusCode();

      if (static_cast<int>(status_code) == 200) {
        std::string body_part;
        std::string result;
        auto deadline = engine::Deadline::FromDuration(kTimeout);
        while (stream_response.ReadChunk(body_part, deadline)) {
          result += body_part;
        }
        return result;
      }
    } catch (const clients::http::TimeoutException&) {
    }
  }

  UINVARIANT(false, "No alive servers");
}

}  // namespace sample

namespace sample2 {

/// [HTTP Client - reuse async]
std::string DifferentUrlsRetry(std::string data, clients::http::Client& http,
                               std::chrono::milliseconds timeout,
                               std::initializer_list<std::string> urls_list) {
  auto request = http.CreateRequest()
                     .post()
                     .data(std::move(data))  // no copying
                     .retry(1)
                     .http_version(clients::http::HttpVersion::k11)
                     .timeout(timeout);

  for (const auto& url : urls_list) {
    request.url(url);  // set URL
    auto future = request.async_perform();

    // ... do something while the request if being performed

    try {
      auto res = future.Get();
      if (res->IsOk()) {
        return std::move(*res).body();  // no copying
      }
    } catch (const clients::http::TimeoutException&) {
    }
  }

  throw std::runtime_error("No alive servers");
}
/// [HTTP Client - reuse async]

}  // namespace sample2

}  // namespace

UTEST(HttpClient, PostEcho) {
  EchoCallback cb;
  const utest::SimpleServer http_server{cb};
  auto http_client_ptr = utest::CreateHttpClient();

  auto request = http_client_ptr->CreateRequest()
                     .post(http_server.GetBaseUrl(), kTestData)
                     .retry(1)
                     .verify(true)
                     .http_version(clients::http::HttpVersion::k11)
                     .timeout(kTimeout);
  {
    const auto res = request.perform();

    EXPECT_EQ(res->body(), kTestData);

    const auto stats = res->GetStats();
    EXPECT_EQ(stats.retries_count, 0);
    EXPECT_GE(stats.open_socket_count, 1);
    EXPECT_GT(stats.time_to_process, std::chrono::seconds(0));
    EXPECT_GT(stats.time_to_connect, std::chrono::seconds(0));

    EXPECT_LT(stats.time_to_process, kTimeout);
    EXPECT_LT(stats.time_to_connect, kTimeout);

    EXPECT_EQ(*cb.responses_200, 1);
  }
  {
    const auto res2 = request.perform();

    EXPECT_EQ(res2->body(), kTestData);

    const auto stats = res2->GetStats();
    EXPECT_EQ(stats.retries_count, 0);
    EXPECT_GE(stats.open_socket_count, 1);
    EXPECT_GT(stats.time_to_process, std::chrono::seconds(0));
    EXPECT_GT(stats.time_to_connect, std::chrono::seconds(0));

    EXPECT_LT(stats.time_to_process, kTimeout);
    EXPECT_LT(stats.time_to_connect, kTimeout);

    EXPECT_EQ(*cb.responses_200, 2);
  }
}

UTEST(HttpClient, StatsOnTimeout) {
  const int kRetries = 5;
  const utest::SimpleServer http_server{&sleep_callback};
  auto http_client_ptr = utest::CreateHttpClient();

  auto request = http_client_ptr->CreateRequest()
                     .post(http_server.GetBaseUrl(), kTestData)
                     .retry(kRetries)
                     .verify(true)
                     .http_version(clients::http::HttpVersion::k11)
                     .timeout(kSmallTimeout);

  try {
    auto res = request.perform();
    FAIL() << "Must throw, but returned " << res->status_code();
  } catch (const clients::http::BaseException& e) {
    EXPECT_EQ(e.GetStats().retries_count, kRetries - 1);
    EXPECT_EQ(e.GetStats().open_socket_count, kRetries);

    EXPECT_GE(e.GetStats().time_to_process, kSmallTimeout);
    EXPECT_LT(e.GetStats().time_to_process, kSmallTimeout * kRetries);
  }

  try {
    auto res = request.perform();
    FAIL() << "Must throw again, but returned " << res->status_code();
  } catch (const clients::http::BaseException& e) {
    EXPECT_EQ(e.GetStats().retries_count, (kRetries - 1) * 2);
    EXPECT_EQ(e.GetStats().open_socket_count, kRetries * 2);

    EXPECT_GE(e.GetStats().time_to_process, kSmallTimeout);
    EXPECT_LT(e.GetStats().time_to_process, kSmallTimeout * kRetries);
  }
}

UTEST(HttpClient, CancelPre) {
  auto task = utils::Async("test", [] {
    const utest::SimpleServer http_server{EchoCallback{}};
    auto http_client_ptr = utest::CreateHttpClient();

    engine::current_task::GetCancellationToken().RequestCancel();

    UEXPECT_THROW(http_client_ptr->CreateRequest(),
                  clients::http::CancelException);
  });

  task.Get();
}

UTEST(HttpClient, CancelPost) {
  auto task = utils::Async("test", [] {
    const utest::SimpleServer http_server{EchoCallback{}};
    auto http_client_ptr = utest::CreateHttpClient();

    auto request = http_client_ptr->CreateRequest()
                       .post(http_server.GetBaseUrl(), kTestData)
                       .timeout(kTimeout);

    engine::current_task::GetCancellationToken().RequestCancel();

    auto future = request.async_perform();
    UEXPECT_THROW(future.Wait(), clients::http::CancelException);
  });

  task.Get();
}

UTEST(HttpClient, CancelRetries) {
  using std::chrono::duration_cast;
  using std::chrono::milliseconds;

  std::atomic<unsigned> server_requests{0};
  unsigned client_retries = 0;

  constexpr unsigned kRetriesCount = 100;
  constexpr unsigned kMinRetries = 3;
  constexpr auto kMaxNonIoReactionTime = std::chrono::seconds{1};
  engine::SingleConsumerEvent enough_retries_event;
  auto callback = [&server_requests,
                   &enough_retries_event](const HttpRequest& request) {
    ++server_requests;
    if (server_requests > kMinRetries) {
      enough_retries_event.Send();
    }
    return sleep_callback_1s(request);
  };

  const utest::SimpleServer http_server{callback};
  auto http_client_ptr = utest::CreateHttpClient();

  const auto start_create_request_time = std::chrono::steady_clock::now();
  auto future = std::make_unique<clients::http::ResponseFuture>(
      http_client_ptr->CreateRequest()
          .post(http_server.GetBaseUrl(), kTestData)
          .retry(kRetriesCount)
          .verify(true)
          .http_version(clients::http::HttpVersion::k11)
          .timeout(kSmallTimeout)
          .async_perform());

  ASSERT_TRUE(enough_retries_event.WaitForEventFor(utest::kMaxTestWaitTime));

  const auto cancellation_start_time = std::chrono::steady_clock::now();
  engine::current_task::GetCancellationToken().RequestCancel();

  const auto request_creation_duration =
      cancellation_start_time - start_create_request_time;

  EXPECT_LT(request_creation_duration, kMaxNonIoReactionTime);

  try {
    [[maybe_unused]] auto val = future->Wait();
    FAIL() << "Must have been cancelled";
  } catch (const clients::http::CancelException& e) {
    client_retries = e.GetStats().retries_count;
    EXPECT_LE(client_retries, kMinRetries * 2);
    EXPECT_GE(client_retries, kMinRetries);
  }

  const auto cancellation_end_time = std::chrono::steady_clock::now();
  const auto cancellation_duration =
      cancellation_end_time - cancellation_start_time;
  EXPECT_LT(cancellation_duration, utest::kMaxTestWaitTime)
      << "Looks like cancel did not cancelled the request, because after the "
         "cancel the request has been working for "
      << duration_cast<milliseconds>(cancellation_duration).count() << "ms";

  future.reset();
  const auto future_destruction_time = std::chrono::steady_clock::now();
  const auto future_destruction_duration =
      future_destruction_time - cancellation_end_time;
  EXPECT_LT(future_destruction_duration, kMaxNonIoReactionTime)
      << "Looks like cancel did not cancelled the request, because after the "
         "cancel future has been destructing for "
      << duration_cast<milliseconds>(future_destruction_duration).count()
      << "ms";

  EXPECT_GE(server_requests, kMinRetries);
  EXPECT_LT(server_requests, kMinRetries * 2);

  EXPECT_LE(server_requests.load(), client_retries + 1)
      << "Cancel() is not fast enough and more than 1 retry was done after "
         "cancellation";
}

UTEST(HttpClient, PostShutdownWithPendingRequest) {
  const utest::SimpleServer http_server{&sleep_callback};
  auto http_client_ptr = utest::CreateHttpClient();

  for (unsigned i = 0; i < kRepetitions; ++i)
    http_client_ptr->CreateRequest()
        .post(http_server.GetBaseUrl(), kTestData)
        .retry(1)
        .verify(true)
        .http_version(clients::http::HttpVersion::k11)
        .timeout(kSmallTimeout)
        .async_perform()
        .Detach();  // Do not do like this in production code!
}

UTEST(HttpClient, PostShutdownWithPendingRequestHuge) {
  // The test produces too much logs otherwise
  auto log_level_scope = LogLevelScope(logging::Level::kError);

  const utest::SimpleServer http_server{&sleep_callback};
  auto http_client_ptr = utest::CreateHttpClient();

  std::string request = kTestData;
  for (unsigned i = 0; i < 20; ++i) {
    request += request;
  }

  for (unsigned i = 0; i < kRepetitions; ++i)
    http_client_ptr->CreateRequest()
        .post(http_server.GetBaseUrl(), request)
        .retry(1)
        .verify(true)
        .http_version(clients::http::HttpVersion::k11)
        .timeout(kSmallTimeout)
        .async_perform()
        .Detach();  // Do not do like this in production code!
}

UTEST(HttpClient, PutEcho) {
  const utest::SimpleServer http_server{EchoCallback{}};
  auto http_client_ptr = utest::CreateHttpClient();

  auto request = http_client_ptr->CreateRequest()
                     .put(http_server.GetBaseUrl(), kTestData)
                     .retry(1)
                     .verify(true)
                     .http_version(clients::http::HttpVersion::k11)
                     .timeout(kTimeout);
  EXPECT_EQ(request.perform()->body(), kTestData);
  EXPECT_EQ(request.perform()->body(), kTestData);
}

UTEST(HttpClient, PutValidateHeader) {
  const utest::SimpleServer http_server{&put_validate_callback};
  auto http_client_ptr = utest::CreateHttpClient();

  auto request = http_client_ptr->CreateRequest()
                     .put(http_server.GetBaseUrl(), kTestData)
                     .retry(1)
                     .verify(true)
                     .http_version(clients::http::HttpVersion::k11)
                     .timeout(kTimeout);

  EXPECT_TRUE(request.perform()->IsOk());
  EXPECT_TRUE(request.perform()->IsOk());
}

UTEST(HttpClient, PutShutdownWithPendingRequest) {
  const utest::SimpleServer http_server{&sleep_callback};
  auto http_client_ptr = utest::CreateHttpClient();

  for (unsigned i = 0; i < kRepetitions; ++i)
    http_client_ptr->CreateRequest()
        .put(http_server.GetBaseUrl(), kTestData)
        .retry(1)
        .verify(true)
        .http_version(clients::http::HttpVersion::k11)
        .timeout(kSmallTimeout)
        .async_perform()
        .Detach();  // Do not do like this in production code!
}

UTEST(HttpClient, PutShutdownWithPendingRequestHuge) {
  // The test produces too much logs otherwise
  auto log_level_scope = LogLevelScope(logging::Level::kError);

  const utest::SimpleServer http_server{&sleep_callback};
  auto http_client_ptr = utest::CreateHttpClient();

  std::string request = kTestData;
  for (unsigned i = 0; i < 20; ++i) {
    request += request;
  }

  for (unsigned i = 0; i < kRepetitions; ++i)
    http_client_ptr->CreateRequest()
        .put(http_server.GetBaseUrl(), request)
        .retry(1)
        .verify(true)
        .http_version(clients::http::HttpVersion::k11)
        .timeout(kSmallTimeout)
        .async_perform()
        .Detach();  // Do not do like this in production code!
}

UTEST(HttpClient, PutShutdownWithHugeResponse) {
  const utest::SimpleServer http_server{&huge_data_callback};
  auto http_client_ptr = utest::CreateHttpClient();

  for (unsigned i = 0; i < kRepetitions; ++i)
    http_client_ptr->CreateRequest()
        .put(http_server.GetBaseUrl(), kTestData)
        .retry(1)
        .verify(true)
        .http_version(clients::http::HttpVersion::k11)
        .timeout(kSmallTimeout)
        .async_perform()
        .Detach();  // Do not do like this in production code!
}

UTEST(HttpClient, MethodsMix) {
  using clients::http::Request;

  const ValidatingSharedCallback callback{};
  const utest::SimpleServer http_server{callback};
  const auto http_client = utest::CreateHttpClient();

  const RequestMethodTestData tests[] = {
      {"PUT", kTestData,
       [](Request& request, const std::string& url,
          std::string data) -> Request& { return request.put(url, data); }},
      {"POST", kTestData,
       [](Request& request, const std::string& url,
          std::string data) -> Request& { return request.post(url, data); }},
      {"GET", "",
       [](Request& request, const std::string& url) -> Request& {
         return request.get(url);
       }},
      {"HEAD", "",
       [](Request& request, const std::string& url) -> Request& {
         return request.head(url);
       }},
      {"DELETE", "",
       [](Request& request, const std::string& url) -> Request& {
         return request.delete_method(url);
       }},
      {"DELETE", "",
       [](Request& request, const std::string& url, std::string data)
           -> Request& { return request.delete_method(url, data); }},
      {"PATCH", kTestData,
       [](Request& request, const std::string& url,
          std::string data) -> Request& { return request.patch(url, data); }},
  };

  for (const auto& method1 : tests) {
    for (const auto& method2 : tests) {
      const bool ok1 = method1
                           .PrepareRequest(http_server.GetBaseUrl(), callback,
                                           http_client->CreateRequest())
                           .perform()
                           ->IsOk();
      EXPECT_TRUE(ok1) << "Failed to perform " << method1.GetMethodName();

      const auto ok2 = method2
                           .PrepareRequest(http_server.GetBaseUrl(), callback,
                                           http_client->CreateRequest())
                           .perform()
                           ->IsOk();
      EXPECT_TRUE(ok2) << "Failed to perform " << method2.GetMethodName()
                       << " after " << method1.GetMethodName();
    }
  }
}

UTEST(HttpClient, MethodsMixReuseRequest) {
  using clients::http::Request;

  const ValidatingSharedCallback callback{};
  const utest::SimpleServer http_server{callback};
  const auto http_client = utest::CreateHttpClient();

  const RequestMethodTestData tests[] = {
      {"PUT", "",
       [](Request& request, const std::string& url) -> Request& {
         return request.put(url);
       }},
      {"POST", "",
       [](Request& request, const std::string& url) -> Request& {
         return request.post(url);
       }},
      {"GET", "",
       [](Request& request, const std::string& url) -> Request& {
         return request.get(url);
       }},
      {"HEAD", "",
       [](Request& request, const std::string& url) -> Request& {
         return request.head(url);
       }},
      {"DELETE", "",
       [](Request& request, const std::string& url) -> Request& {
         return request.delete_method(url);
       }},
      {"DELETE", "",
       [](Request& request, const std::string& url, std::string data)
           -> Request& { return request.delete_method(url, data); }},
      {"PATCH", "",
       [](Request& request, const std::string& url) -> Request& {
         return request.patch(url);
       }},
  };

  for (const auto& method1 : tests) {
    for (const auto& method2 : tests) {
      auto request = method1.PrepareRequest(http_server.GetBaseUrl(), callback,
                                            http_client->CreateRequest());
      EXPECT_TRUE(request.perform()->IsOk())
          << "Failed to perform " << method1.GetMethodName();

      request =
          method2.PrepareRequest(http_server.GetBaseUrl(), callback, request);
      EXPECT_TRUE(request.perform()->IsOk())
          << "Failed to perform " << method2.GetMethodName() << " after "
          << method1.GetMethodName();
    }
  }
}

UTEST(HttpClient, MethodsMixReuseRequestData) {
  using clients::http::Request;

  const ValidatingSharedCallback callback{};
  *callback.data = kTestData;
  const utest::SimpleServer http_server{callback};
  const auto http_client = utest::CreateHttpClient();

  using ZeroArgsMemberFunction = std::function<Request&(Request&)>;
  struct TestData {
    std::string_view method_name;
    ZeroArgsMemberFunction function;
  };

  const TestData tests[] = {
      {"PUT", [](Request& request) -> Request& { return request.put(); }},
      {"POST", [](Request& request) -> Request& { return request.post(); }},
      {"PATCH", [](Request& request) -> Request& { return request.patch(); }},
  };

  auto request = http_client->CreateRequest()
                     .url(http_server.GetBaseUrl())
                     .verify(true)
                     .http_version(clients::http::HttpVersion::k11)
                     .timeout(kTimeout)
                     .data(kTestData);

  for (const auto& [method1, func1] : tests) {
    for (const auto& [method2, func2] : tests) {
      *callback.method_name = method1;
      const auto response1 = func1(request).perform();
      EXPECT_TRUE(response1->IsOk()) << "Failed to perform " << method1;

      *callback.method_name = method2;
      const auto response2 = func2(request).perform();
      EXPECT_TRUE(response2->IsOk())
          << "Failed to perform " << method2 << " after " << method1;
    }
  }
}

UTEST(HttpClient, Headers) {
  const utest::SimpleServer http_server{&header_validate_callback};
  auto http_client_ptr = utest::CreateHttpClient();

  clients::http::Headers headers;
  headers.emplace(kTestHeader, "test");
  headers.emplace(kTestHeaderMixedCase, "notest");  // should be ignored

  for (unsigned i = 0; i < kRepetitions; ++i) {
    auto request = http_client_ptr->CreateRequest()
                       .post(http_server.GetBaseUrl(), kTestData)
                       .retry(1)
                       .headers(headers)
                       .verify(true)
                       .http_version(clients::http::HttpVersion::k11)
                       .timeout(kTimeout);

    EXPECT_TRUE(request.perform()->IsOk());
    EXPECT_TRUE(request.perform()->IsOk());
  }
}

UTEST(HttpClient, HeadersUserAgent) {
  const utest::SimpleServer http_server{&user_agent_validate_callback};
  const utest::SimpleServer http_server_no_ua{&no_user_agent_validate_callback};
  auto http_client_ptr = utest::CreateHttpClient();

  auto request = http_client_ptr->CreateRequest()
                     .post(http_server.GetBaseUrl(), kTestData)
                     .retry(1)
                     .headers({{http::headers::kUserAgent, kTestUserAgent}})
                     .verify(true)
                     .http_version(clients::http::HttpVersion::k11)
                     .timeout(kTimeout);

  auto response = request.perform();
  EXPECT_TRUE(response->IsOk());

  response = request.perform();
  EXPECT_TRUE(response->IsOk());

  response = http_client_ptr->CreateRequest()
                 .post(http_server.GetBaseUrl(), kTestData)
                 .retry(1)
                 .verify(true)
                 .http_version(clients::http::HttpVersion::k11)
                 .timeout(kTimeout)
                 .headers({{http::headers::kUserAgent, "Header to override"}})
                 .headers({{http::headers::kUserAgent, kTestUserAgent}})
                 .perform();

  EXPECT_TRUE(response->IsOk());

  response = http_client_ptr->CreateRequest()
                 .post(http_server_no_ua.GetBaseUrl(), kTestData)
                 .retry(1)
                 .verify(true)
                 .http_version(clients::http::HttpVersion::k11)
                 .timeout(kTimeout)
                 .perform();
  EXPECT_TRUE(response->IsOk());
}

UTEST(HttpClient, Cookies) {
  const auto test = [](const clients::http::Request::Cookies& cookies,
                       std::set<std::string> expected) {
    const utest::SimpleServer http_server{CheckCookie{std::move(expected)}};
    auto http_client_ptr = utest::CreateHttpClient();
    for (unsigned i = 0; i < kRepetitions; ++i) {
      const auto response = http_client_ptr->CreateRequest()
                                .get(http_server.GetBaseUrl())
                                .retry(1)
                                .cookies(cookies)
                                .verify(true)
                                .http_version(clients::http::HttpVersion::k11)
                                .timeout(kTimeout)
                                .perform();
      EXPECT_TRUE(response->IsOk());
    }
  };
  test({{"a", "b"}}, {"a=b"});
  test({{"A", "B"}}, {"A=B"});
  test({{"a", "B"}, {"A", "b"}}, {"a=B", "A=b"});
}

UTEST(HttpClient, HeadersAndWhitespaces) {
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
    const utest::SimpleServer http_server{clients::http::Response200WithHeader{
        std::string(kTestHeader) + ':' + header_value}};

    const auto response = http_client_ptr->CreateRequest()
                              .post(http_server.GetBaseUrl())
                              .timeout(kTimeout)
                              .perform();

    EXPECT_TRUE(response->IsOk()) << "Header value is '" << header_value << "'";
    ASSERT_TRUE(response->headers().count(kTestHeader))
        << "Header value is '" << header_value << "'";
    ASSERT_TRUE(response->headers().count(kTestHeaderMixedCase))
        << "Header value is '" << header_value << "'";
    EXPECT_EQ(response->headers()[kTestHeader], header_data)
        << "Header value is '" << header_value << "'";
    EXPECT_EQ(response->headers()[kTestHeaderMixedCase], header_data)
        << "Header value is '" << header_value << "'";
  }
}

// Make sure that certs are set up and reset on the end of a request.
//
// Smoke test. Fails on MacOS with Segmentation fault while calling
// Request::RequestImpl::on_certificate_request, probably because CURL library
// was misconfigured and uses wrong version of OpenSSL.
UTEST(HttpClient, DISABLED_IN_MAC_OS_TEST_NAME(HttpsWithCert)) {
  auto pkey = crypto::PrivateKey::LoadFromString(kPrivateKey, "");
  auto cert = crypto::Certificate::LoadFromString(kCertificate);
  auto http_client_ptr = utest::CreateHttpClient();
  const utest::SimpleServer http_server{EchoCallback{}};
  const auto url = http_server.GetBaseUrl();
  const auto ssl_url =
      http_server.GetBaseUrl(utest::SimpleServer::Schema::kHttps);

  // Running twice to make sure that after request without a cert the request
  // with a cert succeeds and do not break other request types.
  for (unsigned i = 0; i < 2; ++i) {
    auto response_future = http_client_ptr->CreateRequest()
                               .post(ssl_url)
                               .timeout(kTimeout)
                               .client_key_cert(pkey, cert)
                               .async_perform();

    response_future.Wait();
    UEXPECT_THROW(response_future.Get(), std::exception)
        << "SSL is not used by the server but the request with private key "
           "succeeded";

    auto response = http_client_ptr->CreateRequest()
                        .post(url)
                        .timeout(kTimeout)
                        .client_key_cert(pkey, cert)
                        .perform();

    EXPECT_TRUE(response->IsOk());

    response =
        http_client_ptr->CreateRequest().post(url).timeout(kTimeout).perform();

    EXPECT_TRUE(response->IsOk());
  }
}

UTEST(HttpClient, BasicUsage) {
  auto http_client_ptr = utest::CreateHttpClient();

  const utest::SimpleServer http_server_final{
      clients::http::Response200WithHeader{"xxx: good"}};

  const auto url = http_server_final.GetBaseUrl();
  auto& http_client = *http_client_ptr;
  std::string data{};

  /// [Sample HTTP Client usage]
  const auto response = http_client.CreateRequest()
                            .post(url, data)
                            .timeout(std::chrono::seconds(1))
                            .perform();

  EXPECT_TRUE(response->IsOk());
  /// [Sample HTTP Client usage]
  EXPECT_EQ(response->headers()[std::string_view{"xxx"}], "good");
  EXPECT_EQ(response->headers()[std::string_view{"XXX"}], "good");
}

UTEST(HttpClient, GetWithBody) {
  auto http_client_ptr = utest::CreateHttpClient();

  const utest::SimpleServer http_server_final{
      [](const HttpRequest& request) -> HttpResponse {
        EXPECT_NE(request.find("get_body_data"), std::string::npos);
        return {
            fmt::format(clients::http::kResponse200WithHeaderPattern,
                        "xxx: good"),
            HttpResponse::kWriteAndClose,
        };
      }};

  const auto url = http_server_final.GetBaseUrl();
  auto& http_client = *http_client_ptr;
  std::string data{"get_body_data"};

  const auto response = http_client.CreateRequest()
                            .data(std::move(data))
                            .url(url)
                            .set_custom_http_request_method("GET")
                            .timeout(std::chrono::seconds(1))
                            .perform();

  EXPECT_TRUE(response->IsOk());
  EXPECT_EQ(response->headers()[std::string_view{"xxx"}], "good");
  EXPECT_EQ(response->headers()[std::string_view{"XXX"}], "good");

  // Make sure it doesn't depend on order of get/data
  std::string new_data{"get_body_data"};
  const auto another_response = http_client.CreateRequest()
                                    .url(url)
                                    .set_custom_http_request_method("GET")
                                    .data(std::move(new_data))
                                    .timeout(std::chrono::seconds(1))
                                    .perform();

  EXPECT_TRUE(another_response->IsOk());
  EXPECT_EQ(another_response->headers()[std::string_view{"xxx"}], "good");
  EXPECT_EQ(another_response->headers()[std::string_view{"XXX"}], "good");
}

// Make sure that cURL was build with the fix:
// https://github.com/curl/curl/commit/a12a16151aa33dfd5e7627d4bfc2dc1673a7bf8e
UTEST(HttpClient, RedirectHeaders) {
  auto http_client_ptr = utest::CreateHttpClient();

  const utest::SimpleServer http_server_final{
      clients::http::Response200WithHeader{"xxx: good"}};

  const utest::SimpleServer http_server_redirect{
      Response301WithHeader{http_server_final.GetBaseUrl(), "xxx: bad"}};
  const auto url = http_server_redirect.GetBaseUrl();
  auto& http_client = *http_client_ptr;
  std::string data{};

  const auto response = http_client.CreateRequest()
                            .post(url, data)
                            .timeout(std::chrono::seconds(1))
                            .perform();

  EXPECT_TRUE(response->IsOk())
      << "Looks like you have an outdated version of cURL library. Update "
         "to version 7.72.0 or above is recommended";

  EXPECT_EQ(response->headers()[std::string_view{"xxx"}], "good");
  EXPECT_EQ(response->headers()[std::string_view{"XXX"}], "good");
}

UTEST(HttpClient, BadUrl) {
  auto http_client_ptr = utest::CreateHttpClient();
  auto request = http_client_ptr->CreateRequest();

  const auto check = [&](const std::string& url) {
    [[maybe_unused]] auto response = request.url(url).perform();
    // recreate request if reached here
    request = http_client_ptr->CreateRequest();
  };

  UEXPECT_THROW(check({}), clients::http::BadArgumentException);
  UEXPECT_THROW(check("http://"), clients::http::BadArgumentException);
  UEXPECT_THROW(check("http:\\\\localhost/"),
                clients::http::BadArgumentException);
  UEXPECT_THROW(check("http:///?query"), clients::http::BadArgumentException);
  // three slashes before hostname are apparently okay
  UEXPECT_THROW(check("http:////path/"), clients::http::BadArgumentException);

  UEXPECT_THROW(check("localhost/"), clients::http::BadArgumentException);
  UEXPECT_THROW(check("ftp.localhost/"), clients::http::BadArgumentException);

  UEXPECT_THROW(check("http://localhost:99999/"),
                clients::http::BadArgumentException);
  UEXPECT_THROW(check("http://localhost:abcd/"),
                clients::http::BadArgumentException);
}

UTEST(HttpClient, Retry) {
  auto http_client_ptr = utest::CreateHttpClient();
  const utest::SimpleServer unavail_server{Response503WithConnDrop{}};

  auto response = http_client_ptr->CreateRequest()
                      .get(unavail_server.GetBaseUrl())
                      .timeout(kTimeout)
                      .retry(3)
                      .perform();

  EXPECT_FALSE(response->IsOk());
  EXPECT_EQ(503, response->status_code());
  EXPECT_EQ(2, response->GetStats().retries_count);
}

UTEST(HttpClient, TinyTimeout) {
  auto http_client_ptr = utest::CreateHttpClient();
  const utest::SimpleServer http_server{sleep_callback_1s};

  for (unsigned i = 0; i < kRepetitions; ++i) {
    auto response_future = http_client_ptr->CreateRequest()
                               .post(http_server.GetBaseUrl(), kTestData)
                               .retry(1)
                               .verify(true)
                               .http_version(clients::http::HttpVersion::k11)
                               .timeout(std::chrono::milliseconds(1))
                               .async_perform();

    response_future.Wait();
    UEXPECT_THROW(response_future.Get(), std::exception);
  }
}

UTEST(HttpClient, UsingResolver) {
  const utest::SimpleServer http_server{EchoCallback{},
                                        utest::SimpleServer::kTcpIpV6};

  ResolverWrapper resolver_wrapper;
  auto http_client_ptr =
      utest::CreateHttpClient(resolver_wrapper.fs_task_processor);
  http_client_ptr->SetDnsResolver(&resolver_wrapper.resolver);

  const auto server_url =
      "http://localhost:" + std::to_string(http_server.GetPort());
  auto request = http_client_ptr->CreateRequest()
                     .post(server_url, kTestData)
                     .retry(1)
                     .verify(true)
                     .http_version(clients::http::HttpVersion::k11)
                     .timeout(kTimeout);

  auto res = request.perform();
  EXPECT_EQ(res->body(), kTestData);
}

UTEST(HttpClient, UsingResolverWithIpv6Addrs) {
  const utest::SimpleServer http_server{EchoCallback{},
                                        utest::SimpleServer::kTcpIpV6};

  ResolverWrapper resolver_wrapper;
  auto http_client_ptr =
      utest::CreateHttpClient(resolver_wrapper.fs_task_processor);
  http_client_ptr->SetDnsResolver(&resolver_wrapper.resolver);

  const auto server_url =
      "http://[::1]:" + std::to_string(http_server.GetPort());
  auto request = http_client_ptr->CreateRequest()
                     .post(server_url, kTestData)
                     .retry(1)
                     .verify(true)
                     .http_version(clients::http::HttpVersion::k11)
                     .timeout(kSmallTimeout);

  auto res = request.perform();
  EXPECT_EQ(res->body(), kTestData);
}

UTEST(HttpClient, RequestReuseBasic) {
  EchoCallback shared_echo_callback;
  const utest::SimpleServer http_server{shared_echo_callback,
                                        utest::SimpleServer::kTcpIpV6};

  std::string data = "Some long long request";
  for (unsigned i = 0; i < kFewRepetitions; ++i) {
    data += data;
  }

  auto http_client_ptr = utest::CreateHttpClient();

  const auto server_url =
      "http://localhost:" + std::to_string(http_server.GetPort());
  auto request = http_client_ptr->CreateRequest()
                     .post(server_url, data)
                     .retry(1)
                     .verify(true)
                     .http_version(clients::http::HttpVersion::k11)
                     .timeout(kTimeout);

  std::shared_ptr<clients::http::Response> res;
  for (unsigned i = 0; i < kFewRepetitions; ++i) {
    UEXPECT_NO_THROW(res = request.perform())
        << "Probably your version of cURL is outdated";
    EXPECT_EQ(res->body(), data);
    EXPECT_EQ(res->status_code(), 200);
  }

  EXPECT_EQ(*shared_echo_callback.responses_200, kFewRepetitions);
}

UTEST(HttpClient, RequestReuseSample) {
  EchoCallback shared_echo_callback{};
  const utest::SimpleServer http_server{shared_echo_callback,
                                        utest::SimpleServer::kTcpIpV6};
  const utest::SimpleServer http_sleep_server{sleep_callback_1s};

  std::string data = "Some long long request";
  for (unsigned i = 0; i < kFewRepetitions; ++i) {
    data += data;
  }

  auto http_client_ptr = utest::CreateHttpClient();

  auto resp = sample::DifferentUrlsRetry(data, *http_client_ptr, kSmallTimeout,
                                         {
                                             http_sleep_server.GetBaseUrl(),
                                             http_sleep_server.GetBaseUrl(),
                                             http_server.GetBaseUrl(),
                                             http_server.GetBaseUrl(),
                                         });

  EXPECT_EQ(resp, data);
  EXPECT_EQ(*shared_echo_callback.responses_200, 1);

  resp = sample2::DifferentUrlsRetry(data, *http_client_ptr, kSmallTimeout,
                                     {
                                         http_sleep_server.GetBaseUrl(),
                                         http_sleep_server.GetBaseUrl(),
                                         http_server.GetBaseUrl(),
                                         http_server.GetBaseUrl(),
                                     });

  EXPECT_EQ(resp, data);
  EXPECT_EQ(*shared_echo_callback.responses_200, 2);
}

UTEST(HttpClient, DISABLED_RequestReuseSampleStream) {
  EchoCallback shared_echo_callback{};
  const utest::SimpleServer http_server{shared_echo_callback,
                                        utest::SimpleServer::kTcpIpV6};
  const utest::SimpleServer http_sleep_server{sleep_callback_1s};

  std::string data = "Some long long request";
  for (unsigned i = 0; i < kFewRepetitions; ++i) {
    data += data;
  }

  auto http_client_ptr = utest::CreateHttpClient();
  auto resp = sample::DifferentUrlsRetryStreamResponseBody(
      data, *http_client_ptr, kTimeout,
      {
          http_sleep_server.GetBaseUrl(),
          http_sleep_server.GetBaseUrl(),
          http_server.GetBaseUrl(),
          http_server.GetBaseUrl(),
      });
  EXPECT_EQ(resp, data);
  EXPECT_EQ(*shared_echo_callback.responses_200, 3);
}

UTEST(HttpClient, RequestReuseDifferentUrlAndTimeout) {
  EchoCallback shared_echo_callback;
  const utest::SimpleServer http_echo_server{shared_echo_callback,
                                             utest::SimpleServer::kTcpIpV6};
  const utest::SimpleServer http_sleep_server{sleep_callback_1s};

  auto http_client_ptr = utest::CreateHttpClient();

  auto request = http_client_ptr->CreateRequest()
                     .post(http_sleep_server.GetBaseUrl(), kTestData)
                     .retry(1)
                     .verify(true)
                     .http_version(clients::http::HttpVersion::k11)
                     .timeout(std::chrono::milliseconds(1));

  UEXPECT_THROW(request.perform()->status_code(), std::exception);
  EXPECT_EQ(request.GetUrl(), http_sleep_server.GetBaseUrl());
  EXPECT_EQ(request.GetData(), kTestData);

  request = request.url(http_echo_server.GetBaseUrl()).timeout(kTimeout);
  EXPECT_EQ(request.GetUrl(), http_echo_server.GetBaseUrl());

  for (unsigned i = 0; i < kFewRepetitions; ++i) {
    auto res = request.perform();
    EXPECT_EQ(res->body(), kTestData);
    EXPECT_EQ(200, res->status_code());
  }

  EXPECT_EQ(*shared_echo_callback.responses_200, kFewRepetitions);

  EXPECT_EQ(request.GetUrl(), http_echo_server.GetBaseUrl());
  EXPECT_EQ(request.GetData(), kTestData);
  EXPECT_EQ(request.ExtractData(), kTestData);
  EXPECT_EQ(request.ExtractData(), std::string{});

  auto res = request.data("test").perform();
  EXPECT_EQ(res->body(), "test");
  EXPECT_EQ(200, res->status_code());
  EXPECT_EQ(*shared_echo_callback.responses_200, kFewRepetitions + 1);
}

UTEST(HttpClient, DISABLED_TestsuiteAllowedUrls) {
  auto task = utils::Async("test", [] {
    const utest::SimpleServer http_server{EchoCallback{}};
    auto http_client_ptr = utest::CreateHttpClient();
    http_client_ptr->SetTestsuiteConfig({{"http://126.0.0.1"}, {}});

    EXPECT_NO_THROW((void)http_client_ptr->CreateRequest()
                        .get("http://126.0.0.1")
                        .async_perform());
    UEXPECT_DEATH((void)http_client_ptr->CreateRequest()
                      .get("http://12.0.0.1")
                      .async_perform(),
                  ".*");

    http_client_ptr->SetAllowedUrlsExtra({"http://12.0"});
    EXPECT_NO_THROW((void)http_client_ptr->CreateRequest()
                        .get("http://12.0.0.1")
                        .async_perform());
    UEXPECT_DEATH((void)http_client_ptr->CreateRequest()
                      .get("http://13.0.0.1")
                      .async_perform(),
                  ".*");
  });

  task.Get();
}

UTEST(HttpClient, TestConnectTo) {
  EchoCallback cb;
  const utest::SimpleServer http_server{cb};
  auto http_client_ptr = utest::CreateHttpClient();

  clients::http::ConnectTo connect_to("0.0.0.0:42:127.0.0.1:" +
                                      std::to_string(http_server.GetPort()));
  auto request = http_client_ptr->CreateRequest()
                     .connect_to(connect_to)
                     .post("http://0.0.0.0:42", kTestData)
                     .retry(1)
                     .http_version(clients::http::HttpVersion::k11)
                     .timeout(kTimeout);
  {
    const auto res = request.perform();

    EXPECT_EQ(res->body(), kTestData);
  }
}

UTEST(HttpClient, CheckSchema) {
  auto http_client_ptr = utest::CreateHttpClient();
  UEXPECT_NO_THROW(http_client_ptr->CreateRequest().url("http://localhost"));
  UEXPECT_NO_THROW(http_client_ptr->CreateRequest().url("https://localhost"));
  UEXPECT_NO_THROW(http_client_ptr->CreateRequest().url("httpS://localhost"));
  UEXPECT_NO_THROW(http_client_ptr->CreateRequest().url("HTTP://LOCALHOST"));
  UEXPECT_THROW(http_client_ptr->CreateRequest().url("dict://localhost"),
                clients::http::BadArgumentException);
  UEXPECT_THROW(http_client_ptr->CreateRequest().url("file://localhost"),
                clients::http::BadArgumentException);
  UEXPECT_THROW(http_client_ptr->CreateRequest().url("smtp://localhost"),
                clients::http::BadArgumentException);
  UEXPECT_THROW(http_client_ptr->CreateRequest().url("telnet://localhost"),
                clients::http::BadArgumentException);
}

USERVER_NAMESPACE_END
