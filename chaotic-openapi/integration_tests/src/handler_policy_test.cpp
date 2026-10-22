#include <userver/utest/utest.hpp>

#include <optional>
#include <string>

#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/http/http_request_builder.hpp>
#include <userver/server/request/request_context.hpp>

#include <handlers/policy/hookedpost/handler.hpp>
#include <handlers/policy/plainpost/handler.hpp>
#include <handlers/simple/secretget/handler.hpp>
#include <policy/handler_policy.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using chaotic::openapi::server::impl::ViewDeclaresGetRequestBodyForLogging;
using chaotic::openapi::server::impl::ViewDeclaresGetResponseForLogging;
using chaotic::openapi::server::impl::ViewHasGetRequestBodyForLoggingJson;
using chaotic::openapi::server::impl::ViewHasGetRequestBodyForLoggingString;
using chaotic::openapi::server::impl::ViewHasGetResponseForLogging;

constexpr std::string_view kBody = R"({"value":"secret"})";

std::string Body() { return std::string{kBody}; }

// A near miss: BaseHandler refuses such a View instead of ignoring the hook
struct ViewWithMistypedHook {
    static std::string GetRequestBodyForLogging(const std::string& body, int verbosity);
    static std::string GetResponseForLogging(const std::string& response);
};

static_assert(ViewDeclaresGetRequestBodyForLogging<ViewWithMistypedHook>);
static_assert(!ViewHasGetRequestBodyForLoggingJson<ViewWithMistypedHook>);
static_assert(!ViewHasGetRequestBodyForLoggingString<ViewWithMistypedHook>);
static_assert(ViewDeclaresGetResponseForLogging<ViewWithMistypedHook>);
static_assert(!ViewHasGetResponseForLogging<ViewWithMistypedHook, int>);

}  // namespace

UTEST(HandlerPolicy, PolicyLogsRequestBody) {
    const auto request = server::http::HttpRequestBuilder{}.SetRequestPath("/plain").Build();
    server::request::RequestContext context;

    EXPECT_EQ(
        ::handlers::policy::plainpost::Handler::RequestBodyForLogging(*request, context, Body()),
        "<request redacted by policy>"
    );
}

UTEST(HandlerPolicy, ViewHookTakesPrecedence) {
    const auto request = server::http::HttpRequestBuilder{}.SetRequestPath("/hooked").Build();
    server::request::RequestContext context;

    EXPECT_EQ(
        ::handlers::policy::hookedpost::Handler::RequestBodyForLogging(*request, context, Body()),
        "view saw 1 field(s)"
    );
}

UTEST(HandlerPolicy, PolicyLogsResponseData) {
    const auto request = server::http::HttpRequestBuilder{}.SetRequestPath("/plain").Build();
    server::request::RequestContext context;

    EXPECT_EQ(
        ::handlers::policy::plainpost::Handler::ResponseDataForLogging(*request, context, Body()),
        "<response redacted by policy>"
    );
}

UTEST(HandlerPolicy, PolicyFormatsUndeclaredErrors) {
    const server::handlers::ClientError error{server::handlers::ExternalBody{"bad input"}};

    const auto formatted = test::policy::HandlerPolicy::GetFormattedExternalErrorBody(error);
    ASSERT_TRUE(formatted);
    EXPECT_EQ(formatted->external_body, R"({"error":"bad input"})");
}

UTEST(HandlerPolicy, WithoutPolicyTheDefaultIsKept) {
    const auto request = server::http::HttpRequestBuilder{}.SetRequestPath("/secret").Build();
    server::request::RequestContext context;

    EXPECT_EQ(::handlers::simple::secretget::Handler::RequestBodyForLogging(*request, context, Body()), std::nullopt);
    EXPECT_EQ(::handlers::simple::secretget::Handler::ResponseDataForLogging(*request, context, Body()), std::nullopt);
}

USERVER_NAMESPACE_END
