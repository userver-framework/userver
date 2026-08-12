#include <userver/utest/utest.hpp>

#include <userver/http/common_headers.hpp>
#include <userver/server/http/http_request_builder.hpp>
#include <userver/utest/assert_macros.hpp>

#include <handlers/simple/multipost/handler.hpp>
#include <handlers/simple/multipost/requests.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace multipost = ::handlers::simple::multipost;

UTEST(HandlerBodyMultiContentType, DispatchesJson) {
    auto request =
        server::http::HttpRequestBuilder{}
            .SetBody("\"hello\"")
            .AddHeader(std::string{http::headers::kContentType}, "application/json")
            .Build();

    auto parsed = multipost::ParseRequest(*request, chaotic::openapi::To<multipost::Request>{});
    ASSERT_TRUE(std::holds_alternative<std::string>(parsed.body));
    EXPECT_EQ(std::get<std::string>(parsed.body), "hello");
}

UTEST(HandlerBodyMultiContentType, DispatchesOctetStream) {
    auto request =
        server::http::HttpRequestBuilder{}
            .SetBody("raw bytes")
            .AddHeader(std::string{http::headers::kContentType}, "application/octet-stream")
            .Build();

    auto parsed = multipost::ParseRequest(*request, chaotic::openapi::To<multipost::Request>{});
    ASSERT_TRUE(std::holds_alternative<multipost::RequestBodyApplicationOctetStream>(parsed.body));
    EXPECT_EQ(std::get<multipost::RequestBodyApplicationOctetStream>(parsed.body).data, "raw bytes");
}

UTEST(HandlerBodyMultiContentType, DispatchesJsonWithCharsetParam) {
    auto request =
        server::http::HttpRequestBuilder{}
            .SetBody("\"world\"")
            .AddHeader(std::string{http::headers::kContentType}, "application/json; charset=utf-8")
            .Build();

    auto parsed = multipost::ParseRequest(*request, chaotic::openapi::To<multipost::Request>{});
    ASSERT_TRUE(std::holds_alternative<std::string>(parsed.body));
    EXPECT_EQ(std::get<std::string>(parsed.body), "world");
}

UTEST(HandlerBodyMultiContentType, ThrowsOnUnknownContentType) {
    auto request =
        server::http::HttpRequestBuilder{}
            .SetBody("something")
            .AddHeader(std::string{http::headers::kContentType}, "text/plain")
            .Build();

    UEXPECT_THROW(
        multipost::ParseRequest(*request, chaotic::openapi::To<multipost::Request>{}),
        server::handlers::ClientError
    );
}

}  // namespace

USERVER_NAMESPACE_END
