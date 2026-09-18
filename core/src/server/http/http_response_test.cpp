#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>
#include <gmock/gmock.h>

#include <engine/io/tests/net_listener.hpp>
#include <userver/engine/async.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/server/http/http_request_builder.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/utest/utest.hpp>

#include <server/request/response_data_accounter.hpp>

#include <server/http/http_response_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct ResponseContext final {
    std::shared_ptr<server::http::HttpRequest> request;
    server::http::HttpResponseImpl& response;

    explicit ResponseContext(server::request::ResponseDataAccounter& accounter)
        : request(server::http::HttpRequestBuilder{accounter}.Build()),
          response(server::http::GetHttpResponseImpl(*request))
    {}
};

}  // namespace

TEST(ResponseDataAccounter, StartAndStopRequest) {
    server::request::ResponseDataAccounter accounter;
    const auto now = std::chrono::steady_clock::now();

    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 0);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 0);

    accounter.StartRequest(now);
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 0);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 1);

    accounter.StartRequest(now);
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 0);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 2);

    accounter.StopRequest(0, now);
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 0);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 1);

    accounter.StopRequest(0, now);
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 0);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 0);
}

TEST(ResponseDataAccounter, ReaccountRequestKeepsCount) {
    server::request::ResponseDataAccounter accounter;
    const auto t0 = std::chrono::steady_clock::now();
    const auto t1 = t0 + std::chrono::milliseconds{5};

    accounter.StartRequest(t0);
    ASSERT_EQ(accounter.GetPendingResponsesCount(), 1);
    ASSERT_EQ(accounter.GetPendingResponsesSizeInBytes(), 0);

    accounter.ReaccountRequest(0, t0, 40, t1);
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 40);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 1);

    accounter.ReaccountRequest(40, t1, 7, t1);
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 7);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 1);

    accounter.StopRequest(7, t1);
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 0);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 0);
}

TEST(HttpResponse, AccounterStartsOnConstruction) {
    server::request::ResponseDataAccounter accounter;

    {
        const ResponseContext context{accounter};
        EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 0);
        EXPECT_EQ(accounter.GetPendingResponsesCount(), 1);
        EXPECT_TRUE(context.response.GetData().empty());
    }

    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 0);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 0);
}

TEST(HttpResponse, AccounterTracksSetData) {
    server::request::ResponseDataAccounter accounter;
    ResponseContext context{accounter};

    const std::string body = "test data";
    context.response.SetData(body);

    EXPECT_EQ(context.response.GetData(), body);
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), body.size());
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 1);
}

TEST(HttpResponse, AccounterSetDataReplacesPendingSize) {
    server::request::ResponseDataAccounter accounter;
    ResponseContext context{accounter};

    context.response.SetData("hi");
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 2);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 1);

    context.response.SetData("hello world");
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 11);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 1);

    context.response.SetData("");
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 0);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 1);
}

TEST(HttpResponse, AccounterStopsOnSetSent) {
    server::request::ResponseDataAccounter accounter;
    ResponseContext context{accounter};

    const std::string body = "payload";
    context.response.SetData(body);
    ASSERT_EQ(accounter.GetPendingResponsesSizeInBytes(), body.size());
    ASSERT_EQ(accounter.GetPendingResponsesCount(), 1);

    context.response.SetSent(body.size());

    EXPECT_TRUE(context.response.IsSent());
    EXPECT_EQ(context.response.GetBytesSent(), body.size());
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 0);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 0);
}

TEST(HttpResponse, AccounterStopsOnSetSendFailed) {
    server::request::ResponseDataAccounter accounter;
    {
        ResponseContext context{accounter};
        context.response.SetData("payload");
        ASSERT_EQ(accounter.GetPendingResponsesCount(), 1);
        ASSERT_EQ(accounter.GetPendingResponsesSizeInBytes(), 7);
    }
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 0);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 0);
}

TEST(HttpResponse, AccounterMultipleResponses) {
    server::request::ResponseDataAccounter accounter;

    auto first = std::make_unique<ResponseContext>(accounter);
    first->response.SetData("aaa");
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 3);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 1);

    auto second = std::make_unique<ResponseContext>(accounter);
    second->response.SetData("bbbb");
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 7);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 2);

    first->response.SetSent(3);
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 4);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 1);

    second.reset();
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), 0);
    EXPECT_EQ(accounter.GetPendingResponsesCount(), 0);
}

TEST(HttpResponse, IsLimitReached) {
    server::request::ResponseDataAccounter accounter;
    accounter.SetMaxPendingResponsesSizeInBytes(10);

    ResponseContext small{accounter};
    EXPECT_FALSE(small.response.IsLimitReached());
    small.response.SetData(std::string(9, 'x'));
    EXPECT_FALSE(small.response.IsLimitReached());
    small.response.SetSent(9);

    ResponseContext exact{accounter};
    exact.response.SetData(std::string(10, 'x'));
    EXPECT_TRUE(exact.response.IsLimitReached());
}

TEST(HttpResponse, SetSharedDataAvoidsCopy) {
    server::request::ResponseDataAccounter accounter;
    ResponseContext context{accounter};

    auto body = std::make_shared<const std::string>("shared-payload");
    const auto* data_ptr = body->data();
    context.response.SetSharedData(body);

    EXPECT_EQ(context.response.GetData(), "shared-payload");
    EXPECT_EQ(context.response.GetData().data(), data_ptr);
    EXPECT_EQ(accounter.GetPendingResponsesSizeInBytes(), body->size());

    auto extracted = context.response.ExtractData();
    EXPECT_EQ(extracted.View(), "shared-payload");
    EXPECT_EQ(extracted.View().data(), data_ptr);
}

TEST(HttpResponse, ExtractDataMovesOwnedBody) {
    server::request::ResponseDataAccounter accounter;
    ResponseContext context{accounter};

    context.response.SetData("owned-payload");
    auto extracted = context.response.ExtractData();
    EXPECT_EQ(extracted.View(), "owned-payload");
    EXPECT_TRUE(context.response.GetData().empty());
}

UTEST(HttpResponse, Smoke) {
    const auto test_deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

    server::request::ResponseDataAccounter accounter;
    auto request = server::http::HttpRequestBuilder{accounter}.Build();
    auto& response = server::http::GetHttpResponseImpl(*request);

    constexpr std::string_view kBody = "test data";
    response.SetData(std::string{kBody});
    response.SetStatus(server::http::HttpStatus::kOk);

    auto [server, client] = engine::io::tests::TcpListener{}.MakeSocketPair(test_deadline);
    auto send_task = engine::AsyncNoTracing(
        [](auto&& response, auto&& socket) { response.SendResponse(socket); },
        std::ref(response),
        std::move(server)
    );

    std::vector<char> buffer(4096, '\0');
    const auto reply_size = client.RecvAll(buffer.data(), buffer.size(), test_deadline);

    const std::string_view reply{buffer.data(), reply_size};
    constexpr std::string_view expected_header = "HTTP/1.1 200 OK\r\n";
    ASSERT_EQ(reply.substr(0, expected_header.size()), expected_header);
    const auto expected_content_length = fmt::format("\r\n{}: {}\r\n", http::headers::kContentLength, kBody.size());
    EXPECT_THAT(reply, testing::HasSubstr(expected_content_length));

    EXPECT_EQ(reply.substr(reply.size() - 4 - kBody.size()), fmt::format("\r\n\r\n{}", kBody));
}

UTEST(HttpResponse, AccounterLifetimeIfNotSent) {
    auto accounter = std::make_unique<server::request::ResponseDataAccounter>();
    const auto request = server::http::HttpRequestBuilder{*accounter}.Build();
    server::http::GetHttpResponseImpl(*request).SetSendFailed();
    accounter.reset();
    // Now we just should not crash
}

UTEST(HttpResponse, AccounterLifetimeIfSent) {
    const auto test_deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
    auto accounter = std::make_unique<server::request::ResponseDataAccounter>();

    const auto request = server::http::HttpRequestBuilder{*accounter}.Build();
    auto& response = server::http::GetHttpResponseImpl(*request);

    const std::string body = "test data";
    response.SetData(body);
    response.SetStatus(server::http::HttpStatus::kOk);

    auto [server, client] = engine::io::tests::TcpListener{}.MakeSocketPair(test_deadline);
    auto send_task = engine::AsyncNoTracing(
        [](auto&& response, auto&& socket) { response.SendResponse(socket); },
        std::ref(response),
        std::move(server)
    );

    std::string buffer(4096, '\0');
    const auto reply_size = client.RecvAll(buffer.data(), buffer.size(), test_deadline);
    buffer.resize(reply_size);

    EXPECT_THAT(buffer, testing::HasSubstr(body));

    accounter.reset();
    // Now we just should not crash
}

class HttpResponseBody : public testing::TestWithParam<int> {};

UTEST_P(HttpResponseBody, ForbiddenBody) {
    const auto test_deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

    server::request::ResponseDataAccounter accounter;
    auto request = server::http::HttpRequestBuilder{accounter}.Build();
    auto& response = server::http::GetHttpResponseImpl(*request);

    response.SetData("test data");
    response.SetStatus(static_cast<server::http::HttpStatus>(GetParam()));

    auto [server, client] = engine::io::tests::TcpListener{}.MakeSocketPair(test_deadline);
    auto send_task = engine::AsyncNoTracing(
        [](auto&& response, auto&& socket) { response.SendResponse(socket); },
        std::ref(response),
        std::move(server)
    );

    std::vector<char> buffer(4096, '\0');
    const auto reply_size = client.RecvAll(buffer.data(), buffer.size(), test_deadline);

    const std::string_view reply{buffer.data(), reply_size};
    const std::string expected_header = "HTTP/1.1 " + std::to_string(GetParam()) + " ";
    ASSERT_EQ(reply.substr(0, expected_header.size()), expected_header);
    EXPECT_THAT(reply, testing::Not(testing::HasSubstr(http::headers::kContentLength)));
    EXPECT_EQ(reply.substr(reply.size() - 4), "\r\n\r\n");
}

INSTANTIATE_UTEST_SUITE_P(HttpResponseForbiddenBody, HttpResponseBody, testing::Values(100, 101, 150, 199, 304, 204));

TEST(HttpResponse, GetHeaderDoesntThrow) {
    server::request::ResponseDataAccounter accounter{};
    auto request = server::http::HttpRequestBuilder{accounter}.Build();
    const auto& response = request->GetHttpResponse();

    const auto& header = response.GetHeader("nonexistent-header");
    EXPECT_TRUE(header.empty());
}

USERVER_NAMESPACE_END
