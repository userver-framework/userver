#include <server/http/handler_info_index.hpp>
#include <server/http/http2_session.hpp>
#include <server/http/http_request_parser.hpp>
#include <server/net/stats.hpp>

#include <fmt/format.h>
#include <nghttp2/nghttp2.h>
#include <curl-ev/easy.hpp>

#include <userver/clients/http/client.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/http/common_headers.hpp>

#include <userver/utest/http_client.hpp>
#include <userver/utest/simple_server.hpp>
#include <userver/utest/utest.hpp>

#include "create_parser_test.hpp"

#include <iostream>

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace {

using MockHttpRequest = utest::SimpleServer::Request;
using MockHttpResponse = utest::SimpleServer::Response;
using ParsedRequestPtr = std::shared_ptr<request::RequestBase>;
using ParsedRequestImplPtr = std::shared_ptr<HttpRequestImpl>;
using RequestsQueue = concurrent::SpscQueue<ParsedRequestImplPtr>;

}  // namespace

// Fixture - nghttp2_client
// API - create request with custorm headers, type, url+query, body
class Http2SessionTest : public ::testing::Test {
 public:
  Http2SessionTest()
      : ::testing::Test(),
        parser_(CreateTestParser(
            [this](ParsedRequestPtr&& request) {
              NewRequestCallback(std::move(request));
            },
            USERVER_NAMESPACE::http::HttpVersion::k2)),
        parser1_(CreateTestParser(
            [this](ParsedRequestPtr&& request) {
              NewRequestCallback(std::move(request));
            },
            USERVER_NAMESPACE::http::HttpVersion::k11)),
        server_([this](const MockHttpRequest& request) -> MockHttpResponse {
          return ServerHandler(request);
        }),
        client_ptr_(utest::CreateHttpClient()),
        queue_(RequestsQueue::Create()),
        producer_(queue_->GetProducer()) {
    const auto response =
        client_ptr_->CreateRequest()
            .http_version(USERVER_NAMESPACE::http::HttpVersion::k2)
            .get(server_.GetBaseUrl())
            .perform();

    EXPECT_EQ(200, response->status_code());
  }

  const utest::SimpleServer& GetServer() const { return server_; }

  clients::http::Client& GetClient() { return *client_ptr_; }

  RequestsQueue::Consumer GetConsumer() { return queue_->GetConsumer(); }

  void SetSliceSize(int slice_size) { slice_size_ = slice_size; }

 private:
  void NewRequestCallback(ParsedRequestPtr&& request) {
    auto request_impl = std::dynamic_pointer_cast<HttpRequestImpl>(request);
    if (const auto& h = request_impl->GetHeader(
            USERVER_NAMESPACE::http::headers::k2::kHttp2SettingsHeader);
        !h.empty()) {
      is_upgrade_http_ = true;
      dynamic_cast<Http2Session*>(parser_.get())->UpgradeToHttp2(h);
      return;
    }
    UASSERT(request->GetResponse().GetStreamId().has_value());
    cur_stream_id_ = *request->GetResponse().GetStreamId();
    EXPECT_TRUE(producer_.Push(std::move(request_impl)));
  }

  void ParseHttp2Request(const MockHttpRequest& request) {
    if (slice_size_ == -1) {
      if (request.find("HTTP/1.1") != std::string::npos) {
        parser1_->Parse(request);
      } else {
        EXPECT_TRUE(parser_->Parse(request));
      }
    } else {
      UASSERT(slice_size_);
      for (size_t i = 0; i < request.size(); i += slice_size_) {
        const auto slice = std::string_view{request}.substr(i, slice_size_);
        EXPECT_TRUE(parser_->Parse(slice));
      }
    }
  }

  std::string Make200Response() {
    std::string status{":status"};
    std::string status200{"200"};
    std::array<nghttp2_nv, 1> headers = {
        {{reinterpret_cast<std::uint8_t*>(status.data()),
          reinterpret_cast<std::uint8_t*>(status200.data()), status.size(),
          status200.size(), NGHTTP2_NV_FLAG_NONE}}};

    auto session_ptr =
        dynamic_cast<Http2Session*>(parser_.get())->GetNghttp2SessionPtr();
    int rv = nghttp2_submit_response(session_ptr, cur_stream_id_,
                                     headers.data(), headers.size(), nullptr);

    UASSERT(!rv);

    std::string response_buffer{};
    while (nghttp2_session_want_write(session_ptr)) {
      while (true) {
        const uint8_t* data_ptr{nullptr};
        const ssize_t len = nghttp2_session_mem_send(session_ptr, &data_ptr);
        if (len <= 0) {
          break;
        }
        const std::string_view append{reinterpret_cast<const char*>(data_ptr),
                                      static_cast<std::size_t>(len)};
        response_buffer.append(reinterpret_cast<const char*>(data_ptr), len);
      }
    }
    return response_buffer;
  }

  MockHttpResponse ServerHandler(const MockHttpRequest& request) {
    ParseHttp2Request(request);
    std::string response_buffer{};
    if (is_upgrade_http_) {
      response_buffer = std::string{server::http::kSwitchingProtocolResponse} +
                        Make200Response();
      is_upgrade_http_ = false;
    } else {
      response_buffer = Make200Response();
    }
    return {
        response_buffer,
        MockHttpResponse::kWriteAndContinue,
    };
  }

  bool IsNeedToUpgradeHttp() const noexcept { return is_upgrade_http_; }

  uint32_t cur_stream_id_{1};
  int slice_size_{-1};

  HandlerInfoIndex index_;
  request::HttpRequestConfig config_;
  request::ResponseDataAccounter accounter_;
  net::ParserStats stats_;

  std::shared_ptr<request::RequestParser> parser_;
  std::shared_ptr<request::RequestParser> parser1_;

  const utest::SimpleServer server_;

  std::shared_ptr<clients::http::Client> client_ptr_;

  bool is_upgrade_http_{false};
  std::shared_ptr<RequestsQueue> queue_;
  RequestsQueue::Producer producer_;
};

UTEST_F(Http2SessionTest, SimpleRequest) {
  auto& client = GetClient();
  const auto url = GetServer().GetBaseUrl();

  auto consumer = GetConsumer();
  ParsedRequestImplPtr request;

  const auto response =
      client.CreateRequest()
          .http_version(USERVER_NAMESPACE::http::HttpVersion::k2)
          .get(url)
          .perform();
  EXPECT_EQ(200, response->status_code());

  EXPECT_TRUE(consumer.Pop(request));
  EXPECT_EQ(request->GetMethod(), HttpMethod::kGet);
  EXPECT_EQ(request->GetHttpMajor(), 2);
  EXPECT_EQ(request->GetHttpMinor(), 0);
}

UTEST_F(Http2SessionTest, SmallDataParst) {
  // Set a size of the buffer what will be provided to
  // nghttp2_session_mem_recv
  SetSliceSize(1);

  auto& client = GetClient();
  const auto url = GetServer().GetBaseUrl();

  auto consumer = GetConsumer();
  ParsedRequestImplPtr request;

  const auto response =
      client.CreateRequest()
          .http_version(USERVER_NAMESPACE::http::HttpVersion::k2)
          .get(url)
          .perform();
  EXPECT_EQ(200, response->status_code());

  EXPECT_TRUE(consumer.Pop(request));
  EXPECT_EQ(request->GetMethod(), HttpMethod::kGet);
  EXPECT_EQ(request->GetHttpMajor(), 2);
  EXPECT_EQ(request->GetHttpMinor(), 0);
}

UTEST_F(Http2SessionTest, Url) {
  auto& client = GetClient();
  const auto url = GetServer().GetBaseUrl() + "/test_url";

  auto consumer = GetConsumer();
  ParsedRequestImplPtr request;

  const auto response =
      client.CreateRequest()
          .http_version(USERVER_NAMESPACE::http::HttpVersion::k2)
          .get(url)
          .perform();
  EXPECT_EQ(200, response->status_code());

  EXPECT_TRUE(consumer.Pop(request));
  EXPECT_EQ(request->GetUrl(), "/test_url");

  const auto thraling_slash =
      client.CreateRequest()
          .http_version(USERVER_NAMESPACE::http::HttpVersion::k2)
          .get(url + "/")
          .perform();
  EXPECT_EQ(200, response->status_code());

  EXPECT_TRUE(consumer.Pop(request));
  EXPECT_EQ(request->GetUrl(), "/test_url/");
  EXPECT_EQ(request->GetRequestPath(), "/test_url/");
  EXPECT_EQ(request->GetMethod(), HttpMethod::kGet);
}

UTEST_F(Http2SessionTest, Headers) {
  auto& client = GetClient();
  const auto url = GetServer().GetBaseUrl();

  auto consumer = GetConsumer();
  ParsedRequestImplPtr request;

  clients::http::Headers headers{
      std::make_pair(std::string{"test_header"}, std::string{"test_value"}),
      std::make_pair(std::string{"empty_header"}, std::string{""}),
      std::make_pair(std::string{"a"}, std::string{"b"}),
      std::make_pair(std::string{"CAPS"}, std::string{"CAPS"}),
      std::make_pair(std::string{"double"},
                     std::string{"double_value1"}),  // uses only first value
      std::make_pair(std::string{"double"}, std::string{"double_value2"})};

  const auto response =
      client.CreateRequest()
          .http_version(USERVER_NAMESPACE::http::HttpVersion::k2)
          .headers(headers)
          .get(url)
          .perform();
  EXPECT_EQ(200, response->status_code());

  EXPECT_TRUE(consumer.Pop(request));
  EXPECT_EQ(request->GetHeader("test_header"), "test_value");
  EXPECT_EQ(request->GetHeader("empty_header"), "");
  EXPECT_EQ(request->GetHeader("a"), "b");
  EXPECT_EQ(request->GetHeader("CAPS"), "CAPS");
  EXPECT_EQ(request->GetHeader("double"), "double_value1");
  EXPECT_EQ(request->GetMethod(), HttpMethod::kGet);
}

UTEST_F(Http2SessionTest, Body) {
  auto& client = GetClient();
  const auto url = GetServer().GetBaseUrl();

  auto consumer = GetConsumer();
  ParsedRequestImplPtr request;

  std::string data{"test_data"};

  const auto response =
      client.CreateRequest()
          .http_version(USERVER_NAMESPACE::http::HttpVersion::k2)
          .post(url, data)
          .perform();

  EXPECT_EQ(200, response->status_code());

  EXPECT_TRUE(consumer.Pop(request));
  EXPECT_EQ(request->RequestBody(), "test_data");

  std::string empty_data{};
  const auto response2 =
      client.CreateRequest()
          .http_version(USERVER_NAMESPACE::http::HttpVersion::k2)
          .post(url, empty_data)
          .perform();
  EXPECT_EQ(200, response->status_code());

  EXPECT_TRUE(consumer.Pop(request));
  EXPECT_EQ(request->RequestBody(), "");
  EXPECT_EQ(request->GetMethod(), HttpMethod::kPost);
}

UTEST_F(Http2SessionTest, QueryArgs) {
  auto& client = GetClient();
  auto consumer = GetConsumer();
  ParsedRequestImplPtr request;

  const auto response =
      client.CreateRequest()
          .http_version(USERVER_NAMESPACE::http::HttpVersion::k2)
          .get(GetServer().GetBaseUrl() +
               "/foo/bar?query1=value1&query2=value2")
          .perform();

  EXPECT_EQ(200, response->status_code());
  EXPECT_TRUE(consumer.Pop(request));
  EXPECT_EQ(request->GetMethod(), HttpMethod::kGet);
  EXPECT_EQ(request->RequestBody(), "");
  EXPECT_EQ(request->GetUrl(), "/foo/bar?query1=value1&query2=value2");
  EXPECT_EQ(request->ArgCount(), 2);
  EXPECT_EQ(request->GetArg("query1"), "value1");
  EXPECT_EQ(request->GetArg("query2"), "value2");
}

UTEST_F(Http2SessionTest, HeavyHeader) {
  auto& client = GetClient();
  const auto url = GetServer().GetBaseUrl() + "/hello";

  auto consumer = GetConsumer();
  ParsedRequestImplPtr request;

  std::string heavy_header(10000, '!');
  clients::http::Headers headers{
      std::make_pair(std::string{"heavy_header"}, heavy_header)};

  const auto response =
      client.CreateRequest()
          .http_version(USERVER_NAMESPACE::http::HttpVersion::k2)
          .headers(headers)
          .get(url)
          .perform();
  EXPECT_EQ(200, response->status_code());

  EXPECT_TRUE(consumer.Pop(request));
  EXPECT_EQ(request->GetHeader("heavy_header"), heavy_header);
  EXPECT_EQ(request->GetMethod(), HttpMethod::kGet);
}

UTEST_F(Http2SessionTest, ForCurl) {
  auto& client = GetClient();
  const auto url = GetServer().GetBaseUrl() + "/hello";

  auto consumer = GetConsumer();
  ParsedRequestImplPtr request;

  std::string heavy_header(100, '!');
  clients::http::Headers headers{
      std::make_pair(std::string{"heavy_header"}, heavy_header)};

  std::string body{"swag"};
  const auto response =
      client.CreateRequest()
          .http_version(USERVER_NAMESPACE::http::HttpVersion::k2)
          .headers(headers)
          .post(url, body)
          .perform();
  EXPECT_EQ(200, response->status_code());
  EXPECT_TRUE(consumer.Pop(request));
  EXPECT_EQ(request->GetHeader("heavy_header"), heavy_header);
  EXPECT_EQ(request->GetMethod(), HttpMethod::kPost);
}

}  // namespace server::http

USERVER_NAMESPACE_END
