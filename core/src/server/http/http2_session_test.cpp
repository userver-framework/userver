#include <server/http/handler_info_index.hpp>
#include <server/http/http2_session.hpp>
#include <server/http/http_request_parser.hpp>
#include <server/net/connection_config.hpp>
#include <server/net/stats.hpp>

#include <fmt/format.h>
#include <nghttp2/nghttp2.h>
#include <curl-ev/easy.hpp>

#include <userver/clients/http/client.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/utils/fast_scope_guard.hpp>

#include <userver/utest/http_client.hpp>
#include <userver/utest/simple_server.hpp>
#include <userver/utest/utest.hpp>

#include "create_parser_test.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace {

// For use in tests where we don't expect the timeout to expire.
constexpr auto kTimeout = utest::kMaxTestWaitTime;

using MockHttpRequest = utest::SimpleServer::Request;
using MockHttpResponse = utest::SimpleServer::Response;
using ParsedRequestPtr = std::shared_ptr<http::HttpRequest>;
using ParsedRequestImplPtr = std::shared_ptr<HttpRequest>;
using RequestsQueue = concurrent::SpscQueue<ParsedRequestImplPtr>;

}  // namespace

// Fixture - nghttp2_client
// API - create request with custom headers, type, url+query, body
class Http2SessionTest : public ::testing::Test {
public:
    Http2SessionTest()
        : ::testing::Test(),
          parser_http2_(CreateTestParser(
              [this](ParsedRequestPtr&& request) { NewRequestCallback(std::move(request)); },
              USERVER_NAMESPACE::http::HttpVersion::k2
          )),
          parser_http11_(CreateTestParser(
              [this](ParsedRequestPtr&& request) { NewRequestCallback(std::move(request)); },
              USERVER_NAMESPACE::http::HttpVersion::k11
          )),
          client_ptr_(utest::CreateHttpClient()),
          queue_(RequestsQueue::Create()),
          producer_(queue_->GetProducer()),
          server_([this](const MockHttpRequest& request) -> MockHttpResponse { return ServerHandler(request); })
    {
        [[maybe_unused]] const auto response =
            client_ptr_->CreateRequest()
                .http_version(USERVER_NAMESPACE::http::HttpVersion::k2)
                .get(server_.GetBaseUrl())
                .timeout(kTimeout)
                .perform();
    }

    const utest::SimpleServer& GetServer() const { return server_; }

    clients::http::Client& GetClient() { return *client_ptr_; }

    RequestsQueue::Consumer GetConsumer() { return queue_->GetConsumer(); }

    void SetSliceSize(int slice_size) { slice_size_ = slice_size; }

private:
    void NewRequestCallback(ParsedRequestPtr&& request) {
        if (const auto& h = request->GetHeader(USERVER_NAMESPACE::http::headers::k2::kHttp2SettingsHeader); !h.empty())
        {
            is_upgrade_http_ = true;
            dynamic_cast<Http2Session*>(parser_http2_.get())->UpgradeToHttp2(h);
            return;
        }
        UASSERT(request->GetHttpResponse().GetStreamId().has_value());
        cur_stream_id_ = *request->GetHttpResponse().GetStreamId();
        EXPECT_TRUE(producer_.Push(std::move(request)));
    }

    void ParseHttp2Request(const MockHttpRequest& request) {
        if (slice_size_ == -1) {
            if (request.find("HTTP/1.1") != std::string::npos) {
                parser_http11_->Parse(request);
            } else {
                parser_http2_->Parse(request);
            }
        } else {
            UASSERT(slice_size_);
            for (size_t i = 0; i < request.size(); i += slice_size_) {
                const auto slice = std::string_view{request}.substr(i, slice_size_);
                parser_http2_->Parse(slice);
            }
        }
    }

    std::string Make200Response() {
        std::string status{":status"};
        std::string status200{"200"};
        std::array<nghttp2_nv, 1> headers = {
            {{reinterpret_cast<std::uint8_t*>(status.data()),
              reinterpret_cast<std::uint8_t*>(status200.data()),
              status.size(),
              status200.size(),
              NGHTTP2_NV_FLAG_NONE}}
        };

        auto session_ptr = dynamic_cast<Http2Session*>(parser_http2_.get())->GetNghttp2SessionPtr();
        const int rv = nghttp2_submit_response(session_ptr, cur_stream_id_, headers.data(), headers.size(), nullptr);

        UASSERT(!rv);

        std::string response_buffer{};
        while (nghttp2_session_want_write(session_ptr)) {
            while (true) {
                const uint8_t* data_ptr{nullptr};
                const ssize_t len = nghttp2_session_mem_send(session_ptr, &data_ptr);
                if (len <= 0) {
                    break;
                }
                const std::string_view append{reinterpret_cast<const char*>(data_ptr), static_cast<std::size_t>(len)};
                response_buffer.append(reinterpret_cast<const char*>(data_ptr), len);
            }
        }
        return response_buffer;
    }

    MockHttpResponse ServerHandler(const MockHttpRequest& request) {
        ParseHttp2Request(request);
        std::string response_buffer{};
        if (is_upgrade_http_) {
            response_buffer = std::string{server::http::kSwitchingProtocolResponse} + Make200Response();
            is_upgrade_http_ = false;
        } else {
            response_buffer = Make200Response();
        }
        return {
            response_buffer,
            MockHttpResponse::kWriteAndContinue,
        };
    }

    uint32_t cur_stream_id_{1};
    int slice_size_{-1};

    HandlerInfoIndex index_;
    request::HttpRequestConfig config_;
    request::ResponseDataAccounter accounter_;
    net::ParserStats stats_;

    std::shared_ptr<request::RequestParser> parser_http2_;
    std::shared_ptr<request::RequestParser> parser_http11_;

    std::shared_ptr<clients::http::Client> client_ptr_;

    bool is_upgrade_http_{false};
    std::shared_ptr<RequestsQueue> queue_;
    RequestsQueue::Producer producer_;

    const utest::SimpleServer server_;
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
            .timeout(kTimeout)
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
            .timeout(kTimeout)
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
            .timeout(kTimeout)
            .perform();
    EXPECT_EQ(200, response->status_code());

    EXPECT_TRUE(consumer.Pop(request));
    EXPECT_EQ(request->GetUrl(), "/test_url");

    const auto thraling_slash =
        client.CreateRequest()
            .http_version(USERVER_NAMESPACE::http::HttpVersion::k2)
            .get(url + "/")
            .timeout(kTimeout)
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

    const clients::http::Headers headers{
        std::make_pair(std::string{"test_header"}, std::string{"test_value"}),
        std::make_pair(std::string{"empty_header"}, std::string{""}),
        std::make_pair(std::string{"a"}, std::string{"b"}),
        std::make_pair(std::string{"CAPS"}, std::string{"CAPS"}),
        std::make_pair(std::string{"double"}, std::string{"double_value1"}),  // uses only first value
        std::make_pair(std::string{"double"}, std::string{"double_value2"})
    };

    const auto response =
        client.CreateRequest()
            .http_version(USERVER_NAMESPACE::http::HttpVersion::k2)
            .headers(headers)
            .get(url)
            .timeout(kTimeout)
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

    const std::string data{"test_data"};

    const auto response =
        client.CreateRequest()
            .http_version(USERVER_NAMESPACE::http::HttpVersion::k2)
            .post(url, data)
            .timeout(kTimeout)
            .perform();

    EXPECT_EQ(200, response->status_code());

    EXPECT_TRUE(consumer.Pop(request));
    EXPECT_EQ(request->RequestBody(), "test_data");

    const std::string empty_data{};
    const auto response2 =
        client.CreateRequest()
            .http_version(USERVER_NAMESPACE::http::HttpVersion::k2)
            .post(url, empty_data)
            .timeout(kTimeout)
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
            .get(GetServer().GetBaseUrl() + "/foo/bar?query1=value1&query2=value2")
            .timeout(kTimeout)
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

    const std::string heavy_header(10000, '!');
    const clients::http::Headers headers{std::make_pair(std::string{"heavy_header"}, heavy_header)};

    const auto response =
        client.CreateRequest()
            .http_version(USERVER_NAMESPACE::http::HttpVersion::k2)
            .headers(headers)
            .get(url)
            .timeout(kTimeout)
            .perform();
    EXPECT_EQ(200, response->status_code());

    EXPECT_TRUE(consumer.Pop(request));
    EXPECT_EQ(request->GetHeader("heavy_header"), heavy_header);
    EXPECT_EQ(request->GetMethod(), HttpMethod::kGet);
}

namespace {

// No HTTP client we can link against speaks the extended CONNECT of RFC 8441, so the
// requests are produced by a real client-side nghttp2 session wired straight to the
// parser under test.
class Http2TestClient final {
public:
    Http2TestClient() {
        nghttp2_session_callbacks* callbacks{nullptr};
        UINVARIANT(nghttp2_session_callbacks_new(&callbacks) == 0, "Failed to init client callbacks");
        const utils::FastScopeGuard delete_guard{[&callbacks]() noexcept { nghttp2_session_callbacks_del(callbacks); }};

        nghttp2_session* session{nullptr};
        UINVARIANT(nghttp2_session_client_new(&session, callbacks, this) == 0, "Failed to init client session");
        session_ = SessionPtr{session, nghttp2_session_del};

        const int rv = nghttp2_submit_settings(session_.get(), NGHTTP2_FLAG_NONE, nullptr, 0);
        UINVARIANT(rv == 0, "Failed to submit client settings");
    }

    std::int32_t SubmitRequest(const std::vector<nghttp2_nv>& headers, bool end_stream) {
        const auto flags = end_stream ? NGHTTP2_FLAG_END_STREAM : NGHTTP2_FLAG_NONE;
        const std::int32_t stream_id =
            nghttp2_submit_headers(session_.get(), flags, -1, nullptr, headers.data(), headers.size(), nullptr);
        UINVARIANT(stream_id > 0, "Failed to submit client headers");
        return stream_id;
    }

    void SubmitData(std::int32_t stream_id, std::string_view data, bool end_stream) {
        data_to_send_ = data;
        nghttp2_data_provider provider{};
        provider.source.ptr = this;
        provider.read_callback = ReadData;
        const auto flags = end_stream ? NGHTTP2_FLAG_END_STREAM : NGHTTP2_FLAG_NONE;
        const int rv = nghttp2_submit_data(session_.get(), flags, stream_id, &provider);
        UINVARIANT(rv == 0, "Failed to submit client data");
    }

    /// @returns the bytes the client wants to send, to be fed into the parser.
    std::string ExtractOutput() {
        std::string output;
        while (nghttp2_session_want_write(session_.get())) {
            const std::uint8_t* data{nullptr};
            const auto len = nghttp2_session_mem_send(session_.get(), &data);
            if (len <= 0) {
                break;
            }
            output.append(reinterpret_cast<const char*>(data), len);
        }
        return output;
    }

    void Feed(std::string_view data) {
        const auto readlen =
            nghttp2_session_mem_recv(session_.get(), reinterpret_cast<const std::uint8_t*>(data.data()), data.size());
        UINVARIANT(readlen >= 0, "Failed to parse the server output");
    }

    std::uint32_t GetRemoteSetting(nghttp2_settings_id id) const {
        return nghttp2_session_get_remote_settings(session_.get(), id);
    }

private:
    using SessionPtr = std::unique_ptr<nghttp2_session, decltype(&nghttp2_session_del)>;

    static ssize_t
    ReadData(nghttp2_session*, std::int32_t, std::uint8_t* buf, std::size_t length, std::uint32_t* flags, nghttp2_data_source* source, void*) {
        auto& client = *static_cast<Http2TestClient*>(source->ptr);
        const auto size = std::min(length, client.data_to_send_.size());
        std::memcpy(buf, client.data_to_send_.data(), size);
        client.data_to_send_.erase(0, size);
        if (client.data_to_send_.empty()) {
            *flags |= NGHTTP2_DATA_FLAG_EOF;
        }
        return static_cast<ssize_t>(size);
    }

    SessionPtr session_{nullptr, nghttp2_session_del};
    std::string data_to_send_;
};

nghttp2_nv MakeHeader(std::string_view name, std::string_view value) {
    return {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        reinterpret_cast<std::uint8_t*>(const_cast<char*>(name.data())),
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        reinterpret_cast<std::uint8_t*>(const_cast<char*>(value.data())),
        name.size(),
        value.size(),
        NGHTTP2_NV_FLAG_NONE};
}

std::vector<nghttp2_nv> MakeExtendedConnectHeaders(std::string_view path) {
    return {
        MakeHeader(":method", "CONNECT"),
        MakeHeader(":protocol", "websocket"),
        MakeHeader(":scheme", "https"),
        MakeHeader(":path", path),
        MakeHeader(":authority", "localhost"),
        MakeHeader("sec-websocket-version", "13"),
    };
}

}  // namespace

// Drives Http2Session directly: RFC 8441 needs no sockets to be exercised, and the
// `enable_connect_protocol` option has to be flipped per test.
class Http2ExtendedConnectTest : public ::testing::Test {
public:
    void SetUp() override { MakeSession(/*enable_connect_protocol=*/true); }

    void MakeSession(bool enable_connect_protocol) {
        config_.enable_connect_protocol = enable_connect_protocol;
        // SETTINGS are deltas, so a client that already learned the setting from an
        // earlier session would keep it. Both sides start over together.
        client_ = std::make_unique<Http2TestClient>();
        session_ = std::make_unique<Http2Session>(
            index_,
            request_config_,
            config_,
            [this](std::shared_ptr<http::HttpRequest>&& request) { requests_.push_back(std::move(request)); },
            stats_,
            accounter_,
            engine::io::Sockaddr{}
        );
        // The client needs the server SETTINGS before it may use `:protocol` at all.
        client_->Feed(PullServerOutput());
    }

    std::string PullServerOutput() {
        auto* raw_session = session_->GetNghttp2SessionPtr();
        std::string output;
        while (nghttp2_session_want_write(raw_session)) {
            const std::uint8_t* data{nullptr};
            const auto len = nghttp2_session_mem_send(raw_session, &data);
            if (len <= 0) {
                break;
            }
            output.append(reinterpret_cast<const char*>(data), len);
        }
        return output;
    }

    void PumpClientToServer() { EXPECT_TRUE(session_->Parse(client_->ExtractOutput())); }

protected:
    HandlerInfoIndex index_;
    request::HttpRequestConfig request_config_;
    request::ResponseDataAccounter accounter_;
    net::ParserStats stats_;
    net::Http2SessionConfig config_;

    std::unique_ptr<Http2TestClient> client_;
    std::unique_ptr<Http2Session> session_;
    std::vector<std::shared_ptr<http::HttpRequest>> requests_;
};

UTEST_F(Http2ExtendedConnectTest, SettingIsAdvertised) {
    EXPECT_EQ(1, client_->GetRemoteSetting(NGHTTP2_SETTINGS_ENABLE_CONNECT_PROTOCOL));
}

UTEST_F(Http2ExtendedConnectTest, SettingIsNotAdvertisedByDefault) {
    MakeSession(/*enable_connect_protocol=*/false);
    EXPECT_EQ(0, client_->GetRemoteSetting(NGHTTP2_SETTINGS_ENABLE_CONNECT_PROTOCOL));
}

UTEST_F(Http2ExtendedConnectTest, RoutedAsGetWithoutEndStream) {
    client_->SubmitRequest(MakeExtendedConnectHeaders("/chat"), /*end_stream=*/false);
    PumpClientToServer();

    ASSERT_EQ(1, requests_.size());
    const auto& request = *requests_.front();
    EXPECT_EQ(HttpMethod::kGet, request.GetMethod());
    EXPECT_EQ("/chat", request.GetRequestPath());
    EXPECT_TRUE(request.IsWebsocketExtendedConnect());
    EXPECT_EQ("13", request.GetHeader("sec-websocket-version"));
}

UTEST_F(Http2ExtendedConnectTest, DataIsNotARequestBody) {
    const auto stream_id = client_->SubmitRequest(MakeExtendedConnectHeaders("/chat"), /*end_stream=*/false);
    PumpClientToServer();
    ASSERT_EQ(1, requests_.size());

    client_->SubmitData(stream_id, "websocket frame bytes", /*end_stream=*/false);
    PumpClientToServer();

    // The bytes belong to the tunnelled protocol, not to the request.
    EXPECT_EQ("", requests_.front()->RequestBody());
    EXPECT_EQ(1, requests_.size());
}

UTEST_F(Http2ExtendedConnectTest, RejectedWhenDisabled) {
    MakeSession(/*enable_connect_protocol=*/false);

    // The client refuses to use `:protocol` unadvertised, so the pseudo-header has to be
    // smuggled in as a plain CONNECT to reach the parser at all.
    client_->SubmitRequest(
        {MakeHeader(":method", "CONNECT"), MakeHeader(":authority", "localhost")},
        /*end_stream=*/false
    );
    PumpClientToServer();

    EXPECT_TRUE(requests_.empty());
}

UTEST_F(Http2ExtendedConnectTest, PlainConnectIsRejected) {
    client_->SubmitRequest({MakeHeader(":method", "CONNECT"), MakeHeader(":authority", "localhost")}, false);
    PumpClientToServer();

    EXPECT_TRUE(requests_.empty());
}

UTEST_F(Http2SessionTest, ForCurl) {
    auto& client = GetClient();
    const auto url = GetServer().GetBaseUrl() + "/hello";

    auto consumer = GetConsumer();
    ParsedRequestImplPtr request;

    const std::string heavy_header(100, '!');
    const clients::http::Headers headers{std::make_pair(std::string{"heavy_header"}, heavy_header)};

    const std::string body{"swag"};
    const auto response =
        client.CreateRequest()
            .http_version(USERVER_NAMESPACE::http::HttpVersion::k2)
            .headers(headers)
            .post(url, body)
            .timeout(kTimeout)
            .perform();
    EXPECT_EQ(200, response->status_code());
    EXPECT_TRUE(consumer.Pop(request));
    EXPECT_EQ(request->GetHeader("heavy_header"), heavy_header);
    EXPECT_EQ(request->GetMethod(), HttpMethod::kPost);
}

}  // namespace server::http

USERVER_NAMESPACE_END
