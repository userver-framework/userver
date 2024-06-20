#include <server/http/http2_request_parser.hpp>

#include <fmt/format.h>
#include <nghttp2/nghttp2.h>

#include <curl-ev/easy.hpp>
#include <server/http/handler_info_index.hpp>
#include <server/net/stats.hpp>
#include <userver/clients/http/client.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/engine/io/socket.hpp>

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
class Http2RequestParserTest : public ::testing::Test {
 public:
  Http2RequestParserTest()
      : ::testing::Test(),
        parser_(CreateTestParser(
            [this](ParsedRequestPtr&& request) {
              NewRequestCallback(std::move(request));
            },
            utils::http::HttpVersion::k2)),
        server_([this](const MockHttpRequest& request) -> MockHttpResponse {
          LOG_ERROR() << "___SimpleServerRequest___ =" << request;
          return ServerHandler(request);
        }),
        client_ptr_(utest::CreateHttpClient()),
        queue_(RequestsQueue::Create()),
        producer_(queue_->GetProducer()) {
    const auto response = client_ptr_->CreateRequest()
                              .http_version(utils::http::HttpVersion::k2)
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
    cur_stream_id_ = request->GetResponse().GetStreamId().value();
    LOG_ERROR() << "[CUR_STREAM_ID]=" << std::to_string(cur_stream_id_);
    is_upgrade_http_ = request->UpgradeHttp().has_value();
    if (!is_upgrade_http_) {
      EXPECT_TRUE(producer_.Push(std::move(request_impl)));
    } else {
      upgrade_response_ = *request->UpgradeHttp();
    }
  }

  void ParseHttp2Request(const MockHttpRequest& request) {
    if (slice_size_ == -1) {
      EXPECT_TRUE(parser_->Parse(request.data(), request.size()));
    } else {
      UASSERT(slice_size_);
      for (size_t i = 0; i < request.size(); i += slice_size_) {
        std::string slice(request.substr(i, slice_size_));
        EXPECT_TRUE(parser_->Parse(slice.data(), slice.size()));
      }
    }
  }

  std::string Make200Response() {
    std::string status{":status"};
    std::string status200{"200"};
    std::vector<nghttp2_nv> hdrs = {
        {reinterpret_cast<std::uint8_t*>(status.data()),
         reinterpret_cast<std::uint8_t*>(status200.data()), status.size(),
         status200.size(), NGHTTP2_NV_FLAG_NONE}};

    auto session_ptr = dynamic_cast<Http2RequestParser*>(parser_.get())
                           ->GetNghttp2SessionPtr();
    LOG_ERROR() << "[CUR_STREAM_ID]=" << std::to_string(cur_stream_id_);
    int rv = nghttp2_submit_response(session_ptr, cur_stream_id_, hdrs.data(),
                                     hdrs.size(), nullptr);

    UASSERT(!rv);

    std::string response_buffer{};
    while (nghttp2_session_want_write(session_ptr)) {
      const uint8_t* data_ptr{nullptr};
      ssize_t len = nghttp2_session_mem_send(session_ptr, &data_ptr);
      std::string_view append{reinterpret_cast<const char*>(data_ptr),
                              static_cast<std::size_t>(len)};
      LOG_ERROR() << "[APPEND]=" << append;
      response_buffer.append(reinterpret_cast<const char*>(data_ptr), len);
    }
    return response_buffer;
  }

  MockHttpResponse ServerHandler(const MockHttpRequest& request) {
    ParseHttp2Request(request);
    std::string response_buffer{};
    if (is_upgrade_http_) {
      response_buffer = upgrade_response_;
      is_upgrade_http_ = false;
    } else {
      response_buffer = Make200Response();
    }
    LOG_ERROR() << "[SimpleServerResponse]=" << response_buffer;
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

  const utest::SimpleServer server_;

  std::shared_ptr<clients::http::Client> client_ptr_;

  bool is_upgrade_http_{false};
  std::string upgrade_response_{};
  std::shared_ptr<RequestsQueue> queue_;
  RequestsQueue::Producer producer_;
};

UTEST_F(Http2RequestParserTest, SimpleRequest) {
  auto& client = GetClient();
  const auto url = GetServer().GetBaseUrl();

  auto consumer = GetConsumer();
  ParsedRequestImplPtr request;

  const auto response = client.CreateRequest()
                            .http_version(utils::http::HttpVersion::k2)
                            .get(url)
                            .perform();
  EXPECT_EQ(200, response->status_code());

  EXPECT_TRUE(consumer.Pop(request));
  EXPECT_EQ(request->GetMethod(), HttpMethod::kGet);
  EXPECT_EQ(request->GetHttpMajor(), 2);
  EXPECT_EQ(request->GetHttpMinor(), 0);
}

UTEST_F(Http2RequestParserTest, SmallDataParst) {
  // Set size of buffer what will be provided to
  // nghttp2_session_mem_recv
  SetSliceSize(1);

  auto& client = GetClient();
  const auto url = GetServer().GetBaseUrl();

  auto consumer = GetConsumer();
  ParsedRequestImplPtr request;

  const auto response = client.CreateRequest()
                            .http_version(utils::http::HttpVersion::k2)
                            .get(url)
                            .perform();
  EXPECT_EQ(200, response->status_code());

  EXPECT_TRUE(consumer.Pop(request));
  EXPECT_EQ(request->GetMethod(), HttpMethod::kGet);
  EXPECT_EQ(request->GetHttpMajor(), 2);
  EXPECT_EQ(request->GetHttpMinor(), 0);
}

UTEST_F(Http2RequestParserTest, Url) {
  auto& client = GetClient();
  const auto url = GetServer().GetBaseUrl() + "/test_url";

  auto consumer = GetConsumer();
  ParsedRequestImplPtr request;

  const auto response = client.CreateRequest()
                            .http_version(utils::http::HttpVersion::k2)
                            .get(url)
                            .perform();
  EXPECT_EQ(200, response->status_code());

  EXPECT_TRUE(consumer.Pop(request));
  EXPECT_EQ(request->GetUrl(), "/test_url");

  const auto thraling_slash = client.CreateRequest()
                                  .http_version(utils::http::HttpVersion::k2)
                                  .get(url + "/")
                                  .perform();
  EXPECT_EQ(200, response->status_code());

  EXPECT_TRUE(consumer.Pop(request));
  EXPECT_EQ(request->GetUrl(), "/test_url/");
  EXPECT_EQ(request->GetRequestPath(), "/test_url/");
  EXPECT_EQ(request->GetMethod(), HttpMethod::kGet);
}

UTEST_F(Http2RequestParserTest, Headers) {
  auto& client = GetClient();
  const auto url = GetServer().GetBaseUrl();

  auto consumer = GetConsumer();
  ParsedRequestImplPtr request;

  clients::http::Headers headers{
      {"test_header", "test_value"},
      {"empty_header", ""},
      {"a", "b"},
      {":pseudo", "test"},  // Must be empty, because is a pseudo header
      {"CAPS", "CAPS"},
      {"empty", ""},
      {"double", "double_value1"},  // uses only first value
      {"double", "double_value2"}};

  const auto response = client.CreateRequest()
                            .http_version(utils::http::HttpVersion::k2)
                            .headers(headers)
                            .get(url)
                            .perform();
  EXPECT_EQ(200, response->status_code());

  EXPECT_TRUE(consumer.Pop(request));
  EXPECT_EQ(request->GetHeader("test_header"), "test_value");
  EXPECT_EQ(request->GetHeader("empty_header"), "");
  EXPECT_EQ(request->GetHeader("a"), "b");
  EXPECT_EQ(request->GetHeader("empty"), "");
  EXPECT_EQ(request->GetHeader(":pseudo"), "");
  EXPECT_EQ(request->GetHeader("CAPS"), "CAPS");
  EXPECT_EQ(request->GetHeader("double"), "double_value1");
  EXPECT_EQ(request->GetMethod(), HttpMethod::kGet);
}

UTEST_F(Http2RequestParserTest, Body) {
  auto& client = GetClient();
  const auto url = GetServer().GetBaseUrl();

  auto consumer = GetConsumer();
  ParsedRequestImplPtr request;

  std::string data{"test_data"};

  const auto response = client.CreateRequest()
                            .http_version(utils::http::HttpVersion::k2)
                            .post(url, data)
                            .perform();

  EXPECT_EQ(200, response->status_code());

  EXPECT_TRUE(consumer.Pop(request));
  EXPECT_EQ(request->RequestBody(), "test_data");

  std::string empty_data{};
  const auto response2 = client.CreateRequest()
                             .http_version(utils::http::HttpVersion::k2)
                             .post(url, empty_data)
                             .perform();
  EXPECT_EQ(200, response->status_code());

  EXPECT_TRUE(consumer.Pop(request));
  EXPECT_EQ(request->RequestBody(), "");
  EXPECT_EQ(request->GetMethod(), HttpMethod::kPost);
}

UTEST_F(Http2RequestParserTest, QueryArgs) {
  auto& client = GetClient();
  auto consumer = GetConsumer();
  ParsedRequestImplPtr request;

  const auto response = client.CreateRequest()
                            .http_version(utils::http::HttpVersion::k2)
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

UTEST_F(Http2RequestParserTest, HeavyHeader) {
  auto& client = GetClient();
  const auto url = GetServer().GetBaseUrl() + "/hello";

  auto consumer = GetConsumer();
  ParsedRequestImplPtr request;

  std::string heavy_header(10000, '!');
  clients::http::Headers headers{{"heavy_header", heavy_header}};

  const auto response = client.CreateRequest()
                            .http_version(utils::http::HttpVersion::k2)
                            .headers(headers)
                            .get(url)
                            .perform();
  EXPECT_EQ(200, response->status_code());

  EXPECT_TRUE(consumer.Pop(request));
  EXPECT_EQ(request->GetHeader("heavy_header"), heavy_header);
  EXPECT_EQ(request->GetMethod(), HttpMethod::kGet);
}

UTEST_F(Http2RequestParserTest, ForCurl) {
  auto& client = GetClient();
  const auto url = GetServer().GetBaseUrl() + "/hello";

  auto consumer = GetConsumer();
  ParsedRequestImplPtr request;

  std::string heavy_header(100, '!');
  clients::http::Headers headers{{"heavy_header", heavy_header}};

  std::string body{"swag"};
  const auto response = client.CreateRequest()
                            .http_version(utils::http::HttpVersion::k2)
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
