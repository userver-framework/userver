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

#include <iostream>

// This is temprorary
#define MAKE_NV(NAME, VALUE)                                              \
  {                                                                       \
    (uint8_t*)NAME, (uint8_t*)VALUE, sizeof(NAME) - 1, sizeof(VALUE) - 1, \
        NGHTTP2_NV_FLAG_NONE                                              \
  }

// TODO tests for:
//
// Requests types: GET, POST, PUT, HEAD...
// Headers: empty, double, short, long
// Query args: yes, no, duplicate, array, empty
// Reqeust body: yes, no, empty
//
// corrupted requests???
// Uppercase in headers == malformed request
// non-default pseudo-headers
// connection-specific headers
// omiting mandatory pseudo-headers
// Content-Length header != real length
//
// not-permited symbold in headers
// https://datatracker.ietf.org/doc/html/rfc7230#section-3.2
//
// TODO add malformed option to request impl
// PROTOCOL_ERROR for-malformed errors
//
USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace {

using MockHttpRequest = utest::SimpleServer::Request;
using MockHttpResponse = utest::SimpleServer::Response;
using ParsedRequestPtr = std::shared_ptr<request::RequestBase>;
using ParsedRequestImplPtr = std::shared_ptr<HttpRequestImpl>;
using RequestsQueue = concurrent::SpscQueue<ParsedRequestImplPtr>;

constexpr std::string_view kHttp2SettingsHeader{"HTTP2-Settings: "};
constexpr size_t kClientMagicSize{24};

constexpr char kSwitchingProtocolResponse[] =
    "HTTP/1.1 101 Switching Protocols\r\nConnection: Upgrade\r\nUpgrade: "
    "h2c\r\n\r\n";

}  // namespace

// Fixture - nghttp2_client
// API - create request with custorm headers, type, url+query, body

class Http2RequestParserTest : public ::testing::Test {
 public:
  Http2RequestParserTest()
      : ::testing::Test(),
        parser_(index_, config_, stats_, accounter_,
                [this](ParsedRequestPtr&& request) {
                  NewRequestCallback(std::move(request));
                }),
        server_([this](const MockHttpRequest& request) -> MockHttpResponse {
          return ServerHandler(request);
        }),
        client_ptr_(utest::CreateHttpClient()),
        queue_(RequestsQueue::Create()),
        producer_(queue_->GetProducer()) {
    const auto response = client_ptr_->CreateRequest()
                              ->http_version(clients::http::HttpVersion::k2)
                              ->get(server_.GetBaseUrl())
                              ->perform();

    EXPECT_EQ(200, response->status_code());
  }

  const utest::SimpleServer& GetServer() const { return server_; }

  clients::http::Client& GetClient() { return *client_ptr_; }

  RequestsQueue::Consumer GetConsumer() { return queue_->GetConsumer(); }

  void SetSliceSize(int slice_size) { slice_size_ = slice_size; }

 private:
  void NewRequestCallback(ParsedRequestPtr&& request) {
    auto request_impl = std::dynamic_pointer_cast<HttpRequestImpl>(request);
    cur_stream_id_ = request->GetResponse().stream_id;
    EXPECT_TRUE(producer_.Push(std::move(request_impl)));
  }

  bool DoUpgrade(const MockHttpRequest& request) {
    const auto pos = request.find(kHttp2SettingsHeader);

    const size_t shift = pos + kHttp2SettingsHeader.size();
    const std::string_view client_magic(request.data() + shift,
                                        kClientMagicSize);

    return parser_.ApplySettings(
        client_magic, {{NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}});
  }

  void ParseHttp2Request(const MockHttpRequest& request) {
    if (slice_size_ == -1) {
      EXPECT_TRUE(parser_.Parse(request.data(), request.size()));
    } else {
      UASSERT(slice_size_);
      for (size_t i = 0; i < request.size(); i += slice_size_) {
        std::string slice(request.substr(i, slice_size_));
        EXPECT_TRUE(parser_.Parse(slice.data(), slice.size()));
      }
    }
  }

  void CollectResponse(std::string& response_buffer) {
    // This is temprorary
    // NOLINTNEXTLINE
    std::vector<nghttp2_nv> hdrs = {MAKE_NV(":status", "200")};

    auto session_ptr = parser_.SessionData()->GetSession().Lock();
    int rv = nghttp2_submit_response(session_ptr->get(), cur_stream_id_,
                                     hdrs.data(), hdrs.size(), nullptr);

    UASSERT(!rv);

    while (nghttp2_session_want_write(session_ptr->get())) {
      const uint8_t* data_ptr{nullptr};
      ssize_t len = nghttp2_session_mem_send(session_ptr->get(), &data_ptr);
      response_buffer.append(reinterpret_cast<const char*>(data_ptr), len);
    }
  }

  MockHttpResponse ServerHandler(const MockHttpRequest& request) {
    std::string response_buffer;

    if (!upgrade_completed_) {
      EXPECT_TRUE(DoUpgrade(request));
      response_buffer.append(kSwitchingProtocolResponse);
      upgrade_completed_ = true;
    } else {
      ParseHttp2Request(request);
    }

    CollectResponse(response_buffer);

    return {
        response_buffer,
        MockHttpResponse::kWriteAndContinue,
    };
  }

  uint32_t cur_stream_id_{1};
  int slice_size_{-1};

  bool upgrade_completed_{false};

  HandlerInfoIndex index_;
  request::HttpRequestConfig config_;
  request::ResponseDataAccounter accounter_;
  net::ParserStats stats_;

  Http2RequestParser parser_;

  const utest::SimpleServer server_;

  std::shared_ptr<clients::http::Client> client_ptr_;

  std::shared_ptr<RequestsQueue> queue_;
  RequestsQueue::Producer producer_;
};

UTEST_F(Http2RequestParserTest, SimpleRequest) {
  auto& client = GetClient();
  const auto url = GetServer().GetBaseUrl();

  auto consumer = GetConsumer();
  ParsedRequestImplPtr request;

  const auto response = client.CreateRequest()
                            ->http_version(clients::http::HttpVersion::k2)
                            ->get(url)
                            ->perform();
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
                            ->http_version(clients::http::HttpVersion::k2)
                            ->get(url)
                            ->perform();
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
                            ->http_version(clients::http::HttpVersion::k2)
                            ->get(url)
                            ->perform();
  EXPECT_EQ(200, response->status_code());

  EXPECT_TRUE(consumer.Pop(request));
  EXPECT_EQ(request->GetUrl(), "/test_url");

  const auto thraling_slash = client.CreateRequest()
                                  ->http_version(clients::http::HttpVersion::k2)
                                  ->get(url + "/")
                                  ->perform();
  EXPECT_EQ(200, response->status_code());

  EXPECT_TRUE(consumer.Pop(request));
  EXPECT_EQ(request->GetUrl(), "/test_url/");
}

UTEST_F(Http2RequestParserTest, Headers) {
  auto& client = GetClient();
  const auto url = GetServer().GetBaseUrl();

  auto consumer = GetConsumer();
  ParsedRequestImplPtr request;

  clients::http::Headers headers{
      {"test_header", "test_value"}, {"empty_header", ""}, {"a", "b"}};

  const auto response = client.CreateRequest()
                            ->http_version(clients::http::HttpVersion::k2)
                            ->headers(headers)
                            ->get(url)
                            ->perform();
  EXPECT_EQ(200, response->status_code());

  EXPECT_TRUE(consumer.Pop(request));
  EXPECT_EQ(request->GetHeader("test_header"), "test_value");
  EXPECT_EQ(request->GetHeader("empty_header"), "");
  EXPECT_EQ(request->GetHeader("a"), "b");
}

UTEST_F(Http2RequestParserTest, Body) {
  auto& client = GetClient();
  const auto url = GetServer().GetBaseUrl();

  auto consumer = GetConsumer();
  ParsedRequestImplPtr request;

  std::string data{"test_data"};

  const auto response = client.CreateRequest()
                            ->http_version(clients::http::HttpVersion::k2)
                            ->post(url, data)
                            ->perform();

  EXPECT_EQ(200, response->status_code());

  EXPECT_TRUE(consumer.Pop(request));
  EXPECT_EQ(request->RequestBody(), "test_data");

  std::string empty_data{};
  const auto response2 = client.CreateRequest()
                             ->http_version(clients::http::HttpVersion::k2)
                             ->post(url, empty_data)
                             ->perform();
  EXPECT_EQ(200, response->status_code());

  EXPECT_TRUE(consumer.Pop(request));
  EXPECT_EQ(request->RequestBody(), "");
}

}  // namespace server::http

USERVER_NAMESPACE_END
