#include <clients/http/client.hpp>

#include <fmt/format.h>
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
        clients::http::Client::Create(kHttpIoThreads);

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

TEST(HttpClient, Cancel) {
  TestInCoro([] {
    auto task = utils::Async("test", [] {
      const testing::SimpleServer http_server{&echo_callback};
      std::shared_ptr<clients::http::Client> http_client_ptr =
          clients::http::Client::Create(kHttpIoThreads);

      engine::current_task::GetCurrentTaskContext()->RequestCancel(
          engine::Task::CancellationReason::kUserRequest);

      const auto request = http_client_ptr->CreateRequest()
                               ->post(http_server.GetBaseUrl(), kTestData)
                               ->timeout(std::chrono::milliseconds(100));

      auto response = request->perform();
      EXPECT_TRUE(false) << "should have stopped";
    });

    task.Wait();
    EXPECT_EQ(engine::Task::State::kCancelled, task.GetState());
  });
}

TEST(HttpClient, PostShutdownWithPendingRequest) {
  TestInCoro([] {
    const testing::SimpleServer http_server{&sleep_callback};
    std::shared_ptr<clients::http::Client> http_client_ptr =
        clients::http::Client::Create(kHttpIoThreads);

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
        clients::http::Client::Create(kHttpIoThreads);

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
        clients::http::Client::Create(kHttpIoThreads);

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
        clients::http::Client::Create(kHttpIoThreads);

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
        clients::http::Client::Create(kHttpIoThreads);

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
        clients::http::Client::Create(kHttpIoThreads);

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
        clients::http::Client::Create(kHttpIoThreads);

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
    const auto http_client = clients::http::Client::Create(kHttpIoThreads);

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
        clients::http::Client::Create(kHttpIoThreads);

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
    auto http_client_ptr = clients::http::Client::Create(kHttpIoThreads);

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
