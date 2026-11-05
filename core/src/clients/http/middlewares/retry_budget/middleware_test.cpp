#include <userver/utest/utest.hpp>

#include <string>

#include <userver/clients/http/client.hpp>
#include <userver/clients/http/error.hpp>
#include <userver/clients/http/expected_status_codes.hpp>
#include <userver/clients/http/response.hpp>
#include <userver/clients/http/streamed_response.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/http/url.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/http_server_mock.hpp>

#include <clients/http/middlewares/retry_budget/middleware.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::middlewares::retry_budget {

UTEST(RetryBudget, RetryOk) {
    int count = 0;
    const utest::HttpServerMock
        http_server([&count](const utest::HttpServerMock::HttpRequest&) -> utest::HttpServerMock::HttpResponse {
            count++;
            utest::HttpServerMock::HttpResponse response{};
            response.response_status = 500;
            return response;
        });

    Middleware middleware;
    auto http_client = utest::CreateHttpClientWithMiddleware(middleware);
    auto request =
        http_client->CreateRequest().get().url(http_server.GetBaseUrl()).timeout(utest::kMaxTestWaitTime).retry(3);
    [[maybe_unused]] auto response = request.perform();

    EXPECT_EQ(count, 3);
}

UTEST(RetryBudget, NoBudget) {
    int count = 0;
    const utest::HttpServerMock
        http_server([&count](const utest::HttpServerMock::HttpRequest&) -> utest::HttpServerMock::HttpResponse {
            count++;
            utest::HttpServerMock::HttpResponse response{};
            response.response_status = 500;
            return response;
        });

    Middleware middleware;
    auto http_client = utest::CreateHttpClientWithMiddleware(middleware);

    for (auto i = 0; i < 100; i++) {
        auto request =
            http_client->CreateRequest().get().url(http_server.GetBaseUrl()).timeout(utest::kMaxTestWaitTime).retry(3);
        [[maybe_unused]] auto response = request.perform();
    }

    count = 0;
    auto request =
        http_client->CreateRequest().get().url(http_server.GetBaseUrl()).timeout(utest::kMaxTestWaitTime).retry(3);
    [[maybe_unused]] auto response = request.perform();

    EXPECT_EQ(count, 1);
}

class RetryBudgetExpectedStatus : public testing::TestWithParam<bool> {};

UTEST_P(RetryBudgetExpectedStatus, AccountsExpectedResponsesAsSuccessful) {
    constexpr auto kStatus = USERVER_NAMESPACE::http::StatusCode::kNotFound;
    constexpr int kRequestsToExhaustBudget = 100;
    const bool expected = GetParam();
    const utest::HttpServerMock server{[](const auto&) { return utest::HttpServerMock::HttpResponse{404, {}, {}}; }};
    Middleware middleware;
    auto client = utest::CreateHttpClientWithMiddleware(middleware);
    auto request = client->CreateRequest().get(server.GetBaseUrl()).timeout(utest::kMaxTestWaitTime).retry(1);
    ExpectedStatusCodes codes;
    codes.Set(kStatus, expected);
    request.SetExpectedStatusCodes(codes);
    for (int i = 0; i < kRequestsToExhaustBudget; ++i) {
        const auto response = request.perform();
        EXPECT_EQ(response->status_code(), kStatus);
        EXPECT_EQ(response->IsError(), !expected);
        EXPECT_THROW(response->raise_for_status(), HttpClientException);
    }
    auto& budget = middleware.Storage().GetDestination(std::string{
        USERVER_NAMESPACE::http::ExtractHostname(server.GetBaseUrl())
    });
    EXPECT_EQ(budget.CanRetry(), expected);
    request.SetExpectedStatusCodes({});
    EXPECT_TRUE(request.perform()->IsError());
}

INSTANTIATE_UTEST_SUITE_P(ExpectedCodes, RetryBudgetExpectedStatus, testing::Bool());

UTEST(RetryBudget, ExpectedStreamingResponsePreservesBudget) {
    constexpr int kRequests = 3;
    const utest::HttpServerMock server{[](const auto&) { return utest::HttpServerMock::HttpResponse{404, {}, {}}; }};
    Middleware middleware;
    middleware.Storage().SetPolicy({.max_tokens = 2.0f, .token_ratio = 1.0f});
    auto client = utest::CreateHttpClientWithMiddleware(middleware);
    auto request =
        client->CreateRequest()
            .get(server.GetBaseUrl())
            .timeout(utest::kMaxTestWaitTime)
            .retry(1)
            .SetExpectedStatusCodes({USERVER_NAMESPACE::http::StatusCode::kNotFound});
    for (int i = 0; i < kRequests; ++i) {
        auto queue = concurrent::StringStreamQueue::Create();
        auto response = request.async_perform_stream_body(queue);
        EXPECT_EQ(response.StatusCode(), USERVER_NAMESPACE::http::StatusCode::kNotFound);
        std::string chunk;
        while (response.ReadChunk(chunk, engine::Deadline::FromDuration(utest::kMaxTestWaitTime))) {
        }
    }
    auto& budget = middleware.Storage().GetDestination(std::string{
        USERVER_NAMESPACE::http::ExtractHostname(server.GetBaseUrl())
    });
    EXPECT_TRUE(budget.CanRetry());
}

UTEST(RetryBudget, ExpectedServerErrorKeepsRetryPolicy) {
    constexpr int kAttempts = 3;
    int calls = 0;
    const utest::HttpServerMock server{[&calls](const auto&) {
        ++calls;
        return utest::HttpServerMock::HttpResponse{500, {}, {}};
    }};
    Middleware middleware;
    auto client = utest::CreateHttpClientWithMiddleware(middleware);
    const auto response =
        client->CreateRequest()
            .get(server.GetBaseUrl())
            .timeout(utest::kMaxTestWaitTime)
            .retry(kAttempts)
            .SetExpectedStatusCodes({USERVER_NAMESPACE::http::StatusCode::kInternalServerError})
            .perform();
    EXPECT_EQ(calls, kAttempts);
    EXPECT_FALSE(response->IsError());
    EXPECT_THROW(response->raise_for_status(), HttpServerException);
}

}  // namespace clients::http::middlewares::retry_budget

USERVER_NAMESPACE_END
