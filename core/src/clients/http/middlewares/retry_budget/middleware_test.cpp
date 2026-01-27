#include <userver/utest/utest.hpp>

#include <userver/clients/http/client.hpp>
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

}  // namespace clients::http::middlewares::retry_budget

USERVER_NAMESPACE_END
