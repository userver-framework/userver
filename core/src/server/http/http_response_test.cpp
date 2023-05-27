#include <string_view>
#include <vector>

#include <fmt/format.h>
#include <gmock/gmock.h>

#include <server/http/http_request_impl.hpp>
#include <userver/engine/async.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/internal/net/net_listener.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(HttpResponse, Smoke) {
  const auto test_deadline =
      engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  server::request::ResponseDataAccounter accounter;
  server::http::HttpRequestImpl request{accounter};
  server::http::HttpResponse response{request, accounter};

  constexpr std::string_view kBody = "test data";
  response.SetData(std::string{kBody});
  response.SetStatus(server::http::HttpStatus::kOk);

  auto [server, client] =
      internal::net::TcpListener{}.MakeSocketPair(test_deadline);
  auto send_task = engine::AsyncNoSpan(
      [](auto&& response, auto&& socket) { response.SendResponse(socket); },
      std::ref(response), std::move(server));

  std::vector<char> buffer(4096, '\0');
  const auto reply_size =
      client.RecvAll(buffer.data(), buffer.size(), test_deadline);

  std::string_view reply{buffer.data(), reply_size};
  constexpr std::string_view expected_header = "HTTP/1.1 200 OK\r\n";
  ASSERT_EQ(reply.substr(0, expected_header.size()), expected_header);
  const auto expected_content_length = fmt::format(
      "\r\n{}: {}\r\n", http::headers::kContentLength, kBody.size());
  EXPECT_TRUE(reply.find(expected_content_length) != std::string_view::npos);

  EXPECT_EQ(reply.substr(reply.size() - 4 - kBody.size()),
            fmt::format("\r\n\r\n{}", kBody));
}

UTEST(HttpResponse, AccounterLifetimeIfNotSent) {
  auto accounter = std::make_unique<server::request::ResponseDataAccounter>();
  const server::http::HttpRequestImpl request{*accounter};
  request.GetHttpResponse().SetSendFailed(std::chrono::steady_clock::now());
  accounter.reset();
  // Now we just should not crash
}

UTEST(HttpResponse, AccounterLifetimeIfSent) {
  const auto test_deadline =
      engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
  auto accounter = std::make_unique<server::request::ResponseDataAccounter>();

  const server::http::HttpRequestImpl request{*accounter};
  auto& response = request.GetHttpResponse();

  const std::string body = "test data";
  response.SetData(body);
  response.SetStatus(server::http::HttpStatus::kOk);

  auto [server, client] =
      internal::net::TcpListener{}.MakeSocketPair(test_deadline);
  auto send_task = engine::AsyncNoSpan(
      [](auto&& response, auto&& socket) { response.SendResponse(socket); },
      std::ref(response), std::move(server));

  std::string buffer(4096, '\0');
  const auto reply_size =
      client.RecvAll(buffer.data(), buffer.size(), test_deadline);
  buffer.resize(reply_size);

  EXPECT_THAT(buffer, testing::HasSubstr(body));

  accounter.reset();
  // Now we just should not crash
}

class HttpResponseBody : public testing::TestWithParam<int> {};

UTEST_P(HttpResponseBody, ForbiddenBody) {
  const auto test_deadline =
      engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  server::request::ResponseDataAccounter accounter;
  server::http::HttpRequestImpl request{accounter};
  server::http::HttpResponse response{request, accounter};

  response.SetData("test data");
  response.SetStatus(static_cast<server::http::HttpStatus>(GetParam()));

  auto [server, client] =
      internal::net::TcpListener{}.MakeSocketPair(test_deadline);
  auto send_task = engine::AsyncNoSpan(
      [](auto&& response, auto&& socket) { response.SendResponse(socket); },
      std::ref(response), std::move(server));

  std::vector<char> buffer(4096, '\0');
  const auto reply_size =
      client.RecvAll(buffer.data(), buffer.size(), test_deadline);

  std::string_view reply{buffer.data(), reply_size};
  std::string expected_header = "HTTP/1.1 " + std::to_string(GetParam()) + " ";
  ASSERT_EQ(reply.substr(0, expected_header.size()), expected_header);
  EXPECT_TRUE(reply.find(http::headers::kContentLength) ==
              std::string_view::npos);
  EXPECT_EQ(reply.substr(reply.size() - 4), "\r\n\r\n");
}

INSTANTIATE_UTEST_SUITE_P(HttpResponseForbiddenBody, HttpResponseBody,
                          testing::Values(100, 101, 150, 199, 304, 204));

TEST(HttpResponse, GetHeaderDoesntThrow) {
  server::request::ResponseDataAccounter accounter{};
  const server::http::HttpRequestImpl request_impl{accounter};
  const server::http::HttpResponse response{request_impl, accounter};

  const auto& header = response.GetHeader("nonexistent-header");
  EXPECT_TRUE(header.empty());
}

USERVER_NAMESPACE_END
