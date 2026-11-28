#include <userver/utest/utest.hpp>

#include <optional>
#include <string>
#include <type_traits>

#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/http/http_request_builder.hpp>
#include <userver/server/request/request_context.hpp>

#include <handlers/policy/chaotic_handlers_list.hpp>
#include <handlers/policy/hookedpost/handler.hpp>
#include <handlers/policy/plainpost/handler.hpp>
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

namespace plainpost = ::handlers::policy::plainpost;
namespace hookedpost = ::handlers::policy::hookedpost;

using PlainHandler = plainpost::HandlerFor<test::policy::HandlerPolicy>;
using HookedHandler = hookedpost::HandlerFor<test::policy::HandlerPolicy>;

// The policy is chosen where the handlers are registered, per handler or for a whole list
static_assert(std::is_same_v<plainpost::Handler, plainpost::HandlerFor<>>);
static_assert(!std::is_same_v<plainpost::Handler, PlainHandler>);

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

    EXPECT_EQ(PlainHandler::RequestBodyForLogging(*request, context, Body()), "<request redacted by policy>");
}

UTEST(HandlerPolicy, ViewHookTakesPrecedence) {
    const auto request = server::http::HttpRequestBuilder{}.SetRequestPath("/hooked").Build();
    server::request::RequestContext context;

    EXPECT_EQ(HookedHandler::RequestBodyForLogging(*request, context, Body()), "view saw 1 field(s)");
}

UTEST(HandlerPolicy, PolicyLogsResponseData) {
    const auto request = server::http::HttpRequestBuilder{}.SetRequestPath("/plain").Build();
    server::request::RequestContext context;

    EXPECT_EQ(PlainHandler::ResponseDataForLogging(*request, context, Body()), "<response redacted by policy>");
}

UTEST(HandlerPolicy, PolicyFormatsUndeclaredErrors) {
    const server::handlers::ClientError error{server::handlers::ExternalBody{"bad input"}};

    const auto formatted = test::policy::HandlerPolicy::GetFormattedExternalErrorBody(error);
    ASSERT_TRUE(formatted);
    EXPECT_EQ(formatted->external_body, R"({"error":"bad input"})");
}

UTEST(HandlerPolicy, WithoutPolicyTheDefaultIsKept) {
    const auto request = server::http::HttpRequestBuilder{}.SetRequestPath("/plain").Build();
    server::request::RequestContext context;

    EXPECT_EQ(plainpost::Handler::RequestBodyForLogging(*request, context, Body()), std::nullopt);
    EXPECT_EQ(plainpost::Handler::ResponseDataForLogging(*request, context, Body()), std::nullopt);
}

UTEST(HandlerPolicy, ListAcceptsPolicy) {
    const auto with_policy = ::handlers::policy::ChaoticHandlersList<test::policy::HandlerPolicy>();
    const auto without_policy = ::handlers::policy::ChaoticHandlersList();
    EXPECT_TRUE(with_policy.Contains(PlainHandler::kName));
    EXPECT_TRUE(without_policy.Contains(plainpost::Handler::kName));
}

USERVER_NAMESPACE_END
