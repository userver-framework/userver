#include <userver/utest/utest.hpp>

#include <gmock/gmock.h>

#include <userver/server/http/http_request_builder.hpp>

#include <handlers/simple/secretget/handler.hpp>
#include <handlers/simple/secretget/requests.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace secretget = ::handlers::simple::secretget;

UTEST(HandlerHiddenArgs, ContainsHiddenParam) { EXPECT_TRUE(secretget::HiddenQueryArguments::Contains("token")); }

UTEST(HandlerHiddenArgs, DoesNotContainVisibleParam) {
    EXPECT_FALSE(secretget::HiddenQueryArguments::Contains("name"));
}

UTEST(HandlerHiddenArgs, MaskedUrl) {
    auto request =
        server::http::HttpRequestBuilder{}
            .SetRequestPath("/secret")
            .AddRequestArg("token", "s3cr3t")
            .AddRequestArg("name", "alice")
            .Build();

    const auto url = request->GetMaskedUrl(secretget::HiddenQueryArguments::Contains);

    EXPECT_THAT(url, testing::HasSubstr("token=***"));
    EXPECT_THAT(url, testing::HasSubstr("name=alice"));
    EXPECT_THAT(url, testing::Not(testing::HasSubstr("s3cr3t")));
}

// secretget::Handler::GetUrlForLogging delegates to MaskUrlForLogging, which is
// exposed as a static method so it can be tested without component infrastructure.
UTEST(HandlerGetUrlForLogging, MasksHiddenParamAndShowsVisible) {
    auto request =
        server::http::HttpRequestBuilder{}
            .SetRequestPath("/secret")
            .AddRequestArg("token", "s3cr3t")
            .AddRequestArg("name", "alice")
            .Build();

    const auto url = secretget::Handler::MaskUrlForLogging(*request);

    EXPECT_THAT(url, testing::HasSubstr("token=***"));
    EXPECT_THAT(url, testing::HasSubstr("name=alice"));
    EXPECT_THAT(url, testing::Not(testing::HasSubstr("s3cr3t")));
}

}  // namespace

USERVER_NAMESPACE_END
