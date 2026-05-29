#include <userver/utest/utest.hpp>

#include <gmock/gmock.h>

#include <userver/server/http/http_request_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

UTEST(GetMaskedUrl, MasksHiddenArg) {
    auto request =
        server::http::HttpRequestBuilder{}
            .SetRequestPath("/secret")
            .AddRequestArg("token", "s3cr3t")
            .AddRequestArg("name", "alice")
            .Build();

    const auto url = request->GetMaskedUrl([](std::string_view name) { return name == "token"; });

    EXPECT_THAT(url, testing::HasSubstr("token=***"));
    EXPECT_THAT(url, testing::HasSubstr("name=alice"));
    EXPECT_THAT(url, testing::Not(testing::HasSubstr("s3cr3t")));
}

UTEST(GetMaskedUrl, ShowsNonHiddenArgs) {
    auto request =
        server::http::HttpRequestBuilder{}
            .SetRequestPath("/greet")
            .AddRequestArg("name", "bob")
            .AddRequestArg("count", "3")
            .Build();

    const auto url = request->GetMaskedUrl([](std::string_view) { return false; });

    EXPECT_THAT(url, testing::HasSubstr("name=bob"));
    EXPECT_THAT(url, testing::HasSubstr("count=3"));
}

UTEST(GetMaskedUrl, NoArgs) {
    auto request = server::http::HttpRequestBuilder{}.SetRequestPath("/greet").Build();

    const auto url = request->GetMaskedUrl([](std::string_view) { return true; });

    EXPECT_EQ(url, "/greet");
}

UTEST(GetMaskedUrl, EncodesSpecialChars) {
    auto request =
        server::http::HttpRequestBuilder{}
            .SetRequestPath("/search")
            .AddRequestArg("q", "hello world")
            .AddRequestArg("filter", "a=1&b=2")
            .Build();

    const auto url = request->GetMaskedUrl([](std::string_view) { return false; });

    EXPECT_THAT(url, testing::HasSubstr("q=hello%20world"));
    EXPECT_THAT(url, testing::HasSubstr("filter=a%3D1%26b%3D2"));
}

}  // namespace

USERVER_NAMESPACE_END
