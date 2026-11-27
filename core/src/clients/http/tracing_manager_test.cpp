#include <userver/tracing/manager_component.hpp>

#include <chrono>
#include <sstream>
#include <stdexcept>
#include <string>

#include <fmt/format.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <clients/http/client_utils_test.hpp>
#include <clients/http/testsuite.hpp>
#include <logging/logging_test.hpp>
#include <userver/clients/http/client.hpp>
#include <userver/clients/http/error.hpp>
#include <userver/clients/http/expected_status_codes.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/simple_server.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class MockTracingManager : public tracing::TracingManagerBase {
public:
    bool TryFillSpanBuilderFromRequest(const server::http::HttpRequest&, tracing::SpanBuilder&) const override {
        ++create_new_span_counter_;
        return true;
    }

    void FillRequestWithTracingContext(const tracing::Span&, clients::http::MiddlewareRequest) const override {
        ++fill_request_counter_;
    }

    void FillResponseWithTracingContext(const tracing::Span&, server::http::HttpResponse&) const override {
        ++fill_response_counter_;
    }

    size_t GetCreateNewSpanCounter() const { return create_new_span_counter_; }
    size_t GetFillRequestCounter() const { return fill_request_counter_; }
    size_t GetFillResponseCounter() const { return fill_response_counter_; }

private:
    mutable size_t create_new_span_counter_ = 0;
    mutable size_t fill_request_counter_ = 0;
    mutable size_t fill_response_counter_ = 0;
};

}  // namespace

UTEST(TracingManagerBase, TracingManagerCorrectCalls) {
    const MockTracingManager tracing_manager;
    auto http_client_ptr = utest::CreateHttpClient(tracing_manager);

    const utest::SimpleServer http_server_final{clients::http::Response200WithHeader{"xxx: test"}};

    const auto url = http_server_final.GetBaseUrl();
    auto& http_client = *http_client_ptr;
    const std::string data{};

    const auto response = http_client.CreateRequest().post(url, data).timeout(std::chrono::seconds(1)).perform();

    EXPECT_TRUE(response->IsOk());
    EXPECT_EQ(tracing_manager.GetCreateNewSpanCounter(), 0);
    EXPECT_EQ(tracing_manager.GetFillRequestCounter(), 1);
    EXPECT_EQ(tracing_manager.GetFillResponseCounter(), 0);
}

UTEST(TracingManagerBase, TracingManagerCorrectCallsPerRequest) {
    auto http_client_ptr = utest::CreateHttpClient();

    const utest::SimpleServer http_server_final{clients::http::Response200WithHeader{"xxx: test"}};

    const auto url = http_server_final.GetBaseUrl();
    auto& http_client = *http_client_ptr;
    const std::string data{};

    const MockTracingManager tracing_manager;
    const auto response =
        http_client.CreateRequest()
            .post(url, data)
            .timeout(std::chrono::seconds(1))
            .SetTracingManager(tracing_manager)
            .perform();

    EXPECT_TRUE(response->IsOk());
    EXPECT_EQ(tracing_manager.GetCreateNewSpanCounter(), 0);
    EXPECT_EQ(tracing_manager.GetFillRequestCounter(), 1);
    EXPECT_EQ(tracing_manager.GetFillResponseCounter(), 0);
}

class HttpExpectedStatusCodes : public LoggingTest {
protected:
    std::string HttpSpanLog() const {
        logging::LogFlush();
        std::istringstream logs{GetStreamString()};
        std::string line;
        while (std::getline(logs, line)) {
            if (line.find("stopwatch_name=GET /expected-status-code-test\t") != std::string::npos) {
                return line;
            }
        }
        ADD_FAILURE() << "HTTP span not found: " << GetStreamString();
        return {};
    }
};

struct HttpExpectedStatusCodeCase {
    const char* name;
    int status_code;
    clients::http::ExpectedStatusCodes expected_codes;
    bool is_error;
};

class HttpExpectedStatusCodesResponses
    : public HttpExpectedStatusCodes,
      public testing::WithParamInterface<HttpExpectedStatusCodeCase> {};

UTEST_P(HttpExpectedStatusCodesResponses, ClassifiesResponseAndSpan) {
    const auto& param = GetParam();
    auto client = utest::CreateHttpClient();
    const utest::SimpleServer server{[status_code = param.status_code](const auto&) {
        return utest::SimpleServer::Response{
            fmt::format("HTTP/1.1 {} Test\r\nContent-Length: 0\r\nConnection: close\r\n\r\n", status_code)
        };
    }};
    const auto response =
        client->CreateRequest()
            .get(server.GetBaseUrl())
            .SetUrlTemplate("/expected-status-code-test")
            .SetExpectedStatusCodes(param.expected_codes)
            .timeout(utest::kMaxTestWaitTime)
            .retry(1)
            .perform();
    EXPECT_EQ(static_cast<int>(response->status_code()), param.status_code);
    EXPECT_EQ(response->IsError(), param.is_error);
    const auto span_log = HttpSpanLog();
    if (param.is_error) {
        EXPECT_THAT(span_log, testing::HasSubstr("\terror=1\t"));
    } else {
        EXPECT_THAT(span_log, testing::Not(testing::HasSubstr("\terror=")));
    }
}

INSTANTIATE_UTEST_SUITE_P(
    Responses,
    HttpExpectedStatusCodesResponses,
    testing::Values(
        HttpExpectedStatusCodeCase{"Default404IsError", 404, {}, true},
        HttpExpectedStatusCodeCase{
            "Expected404OmitsError",
            404,
            {USERVER_NAMESPACE::http::StatusCode::kNotFound},
            false
        },
        HttpExpectedStatusCodeCase{"Unexpected500IsError", 500, {USERVER_NAMESPACE::http::StatusCode::kNotFound}, true},
        HttpExpectedStatusCodeCase{"SuccessOmitsError", 200, {USERVER_NAMESPACE::http::StatusCode::kNotFound}, false}
    ),
    [](const testing::TestParamInfo<HttpExpectedStatusCodeCase>& info) { return std::string{info.param.name}; }
);

UTEST_F(HttpExpectedStatusCodes, ReplacesExpectedCodesInsteadOfMerging) {
    auto client = utest::CreateHttpClient();
    const utest::SimpleServer server{[](const auto&) {
        return utest::SimpleServer::Response{"HTTP/1.1 404 Test\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"};
    }};
    auto request = client->CreateRequest().get(server.GetBaseUrl()).timeout(utest::kMaxTestWaitTime).retry(1);
    request.SetUrlTemplate("/expected-status-code-test");
    request.SetExpectedStatusCodes({USERVER_NAMESPACE::http::StatusCode::kNotFound});
    EXPECT_FALSE(request.perform()->IsError());
    EXPECT_THAT(HttpSpanLog(), testing::Not(testing::HasSubstr("\terror=")));
    ClearLog();

    request.SetExpectedStatusCodes({USERVER_NAMESPACE::http::StatusCode::kConflict});
    const auto response = request.perform();
    EXPECT_TRUE(response->IsError());
    EXPECT_THROW(response->raise_for_status(), clients::http::HttpClientException);
    EXPECT_THAT(HttpSpanLog(), testing::HasSubstr("\terror=1\t"));
}

UTEST_F(HttpExpectedStatusCodes, EmptySetClearsExpectedCodes) {
    auto client = utest::CreateHttpClient();
    const utest::SimpleServer server{[](const auto&) {
        return utest::SimpleServer::Response{"HTTP/1.1 404 Test\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"};
    }};
    auto request = client->CreateRequest().get(server.GetBaseUrl()).timeout(utest::kMaxTestWaitTime).retry(1);
    request.SetUrlTemplate("/expected-status-code-test");

    //! [http-client-expected-status-codes]
    request.SetExpectedStatusCodes({USERVER_NAMESPACE::http::StatusCode::kNotFound});
    auto response = request.perform();
    //! [http-client-expected-status-codes]
    EXPECT_FALSE(response->IsError());
    EXPECT_THAT(HttpSpanLog(), testing::Not(testing::HasSubstr("\terror=")));
    ClearLog();

    request.SetExpectedStatusCodes({});
    response = request.perform();
    EXPECT_TRUE(response->IsError());
    EXPECT_THROW(response->raise_for_status(), clients::http::HttpClientException);
    EXPECT_THAT(HttpSpanLog(), testing::HasSubstr("\terror=1\t"));
}

UTEST_F(HttpExpectedStatusCodes, TransportErrorIsNeverExpected) {
    auto client = utest::CreateHttpClient();
    const utest::SimpleServer server{[](const auto&) { return utest::SimpleServer::Response{}; }};
    EXPECT_THROW(
        (void)client->CreateRequest()
            .get(server.GetBaseUrl())
            .SetUrlTemplate("/expected-status-code-test")
            .SetExpectedStatusCodes(
                {USERVER_NAMESPACE::http::StatusCode::kNotFound,
                 USERVER_NAMESPACE::http::StatusCode::kInternalServerError,
                 static_cast<USERVER_NAMESPACE::http::StatusCode>(599)}
            )
            .timeout(utest::kMaxTestWaitTime)
            .retry(1)
            .perform(),
        clients::http::BaseException
    );
    EXPECT_THAT(HttpSpanLog(), testing::HasSubstr("\terror=1\t"));
}

UTEST_F(HttpExpectedStatusCodes, TimeoutIsNeverExpected) {
    auto client = utest::CreateHttpClient();
    const utest::SimpleServer server{[](const auto&) {
        return utest::SimpleServer::Response{{}, utest::SimpleServer::Response::kTryReadMore};
    }};
    EXPECT_THROW(
        (void)client->CreateRequest()
            .get(server.GetBaseUrl())
            .SetUrlTemplate("/expected-status-code-test")
            .SetExpectedStatusCodes(
                {USERVER_NAMESPACE::http::StatusCode::kNotFound,
                 USERVER_NAMESPACE::http::StatusCode::kInternalServerError,
                 static_cast<USERVER_NAMESPACE::http::StatusCode>(599)}
            )
            .timeout(std::chrono::milliseconds{50})
            .retry(1)
            .perform(),
        clients::http::TimeoutException
    );
    EXPECT_THAT(HttpSpanLog(), testing::HasSubstr("\terror=1\t"));
}

UTEST(HttpExpectedStatusCodesValidation, RejectsInvalidCodes) {
    auto client = utest::CreateHttpClient();
    auto request = client->CreateRequest();
    EXPECT_THROW(
        request.SetExpectedStatusCodes({static_cast<USERVER_NAMESPACE::http::StatusCode>(99)}),
        std::invalid_argument
    );
    EXPECT_THROW(
        request.SetExpectedStatusCodes({static_cast<USERVER_NAMESPACE::http::StatusCode>(600)}),
        std::invalid_argument
    );
}

UTEST(HttpExpectedStatusCodesValidation, AcceptsValidCodes) {
    auto client = utest::CreateHttpClient();
    auto request = client->CreateRequest();
    EXPECT_NO_THROW(request.SetExpectedStatusCodes(
        {USERVER_NAMESPACE::http::StatusCode::kContinue,
         USERVER_NAMESPACE::http::StatusCode::kNotFound,
         static_cast<USERVER_NAMESPACE::http::StatusCode>(599)}
    ));
}

USERVER_NAMESPACE_END
