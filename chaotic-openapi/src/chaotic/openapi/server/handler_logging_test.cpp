#include <userver/chaotic/openapi/server/handler_base.hpp>

#include <string>

#include <gtest/gtest.h>
#include <userver/server/http/http_request_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace co_server = chaotic::openapi::server;

struct FakeRequest {};
struct FakeResponse {};
struct FakeTag {};

inline constexpr std::string_view kFakeHandlerName = "fake";

using FakeDeps = co_server::dependencies::ForHandler<FakeTag>;

// ---- MinimalView (no logging methods) ----

struct MinimalView {
    static FakeResponse Handle(FakeRequest&&, FakeDeps&&) { return {}; }
};

static_assert(!co_server::impl::ViewHasGetRequestBodyForLoggingJson<MinimalView>);
static_assert(!co_server::impl::ViewHasGetRequestBodyForLoggingString<MinimalView>);
static_assert(!co_server::impl::ViewHasGetInvalidRequestBodyForLogging<MinimalView>);
static_assert(!co_server::impl::ViewHasGetResponseForLogging<MinimalView, FakeResponse>);

using MinimalHandler = co_server::BaseHandler<kFakeHandlerName, void, FakeRequest, FakeResponse, FakeTag, MinimalView>;

TEST(HandlerLogging, MinimalViewReturnsNullopt) {
    auto request = server::http::HttpRequestBuilder{}.Build();
    EXPECT_FALSE(MinimalHandler::TryGetBodyForLogging(*request, "anything").has_value());
}

// ---- JsonBodyView ----

struct JsonBodyView {
    static FakeResponse Handle(FakeRequest&&, FakeDeps&&) { return {}; }

    static std::string GetRequestBodyForLogging(const formats::json::Value& body) {
        return "k=" + body["k"].As<std::string>("");
    }

    static std::string GetInvalidRequestBodyForLogging(const server::http::HttpRequest&) { return "INVALID"; }
};

static_assert(co_server::impl::ViewHasGetRequestBodyForLoggingJson<JsonBodyView>);
static_assert(!co_server::impl::ViewHasGetRequestBodyForLoggingString<JsonBodyView>);
static_assert(co_server::impl::ViewHasGetInvalidRequestBodyForLogging<JsonBodyView>);

using JsonHandler = co_server::BaseHandler<kFakeHandlerName, void, FakeRequest, FakeResponse, FakeTag, JsonBodyView>;

TEST(HandlerLogging, JsonBodyValidRequest) {
    auto request = server::http::HttpRequestBuilder{}.Build();
    auto result = JsonHandler::TryGetBodyForLogging(*request, R"({"k":"v"})");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "k=v");
}

TEST(HandlerLogging, JsonBodyInvalidThrows) {
    auto request = server::http::HttpRequestBuilder{}.Build();
    EXPECT_THROW(JsonHandler::TryGetBodyForLogging(*request, "not-json"), std::exception);
}

// ---- StringBodyView ----

struct StringBodyView {
    static FakeResponse Handle(FakeRequest&&, FakeDeps&&) { return {}; }

    static std::string GetRequestBodyForLogging(const std::string& body) {
        return "len=" + std::to_string(body.size());
    }
};

static_assert(!co_server::impl::ViewHasGetRequestBodyForLoggingJson<StringBodyView>);
static_assert(co_server::impl::ViewHasGetRequestBodyForLoggingString<StringBodyView>);
static_assert(!co_server::impl::ViewHasGetInvalidRequestBodyForLogging<StringBodyView>);

using StringHandler =
    co_server::BaseHandler<kFakeHandlerName, void, FakeRequest, FakeResponse, FakeTag, StringBodyView>;

TEST(HandlerLogging, StringBody) {
    auto request = server::http::HttpRequestBuilder{}.Build();
    auto result = StringHandler::TryGetBodyForLogging(*request, "hello");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "len=5");
}

// ---- ResponseLoggingView ----

struct ResponseLoggingView {
    static FakeResponse Handle(FakeRequest&&, FakeDeps&&) { return {}; }

    static std::string
    GetResponseForLogging(const FakeResponse&, const std::string& serialized, server::request::RequestContext&) {
        return "resp:" + serialized;
    }
};

static_assert(co_server::impl::ViewHasGetResponseForLogging<ResponseLoggingView, FakeResponse>);
static_assert(!co_server::impl::ViewHasGetResponseForLogging<MinimalView, FakeResponse>);

TEST(HandlerLogging, ResponseForLogging) {
    server::request::RequestContext context;
    const std::string serialized = R"({"ok":true})";
    const auto result = ResponseLoggingView::GetResponseForLogging(FakeResponse{}, serialized, context);
    EXPECT_EQ(result, "resp:" + serialized);
}

}  // namespace

USERVER_NAMESPACE_END
