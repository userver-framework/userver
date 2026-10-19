#include <userver/server/http/http_method.hpp>

#include <userver/utest/utest.hpp>

#include <server/http/handler_methods.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
namespace sh = server::http;
}

UTEST(ServerHttpMethodTest, ToString) {
    EXPECT_EQ(sh::ToString(sh::HttpMethod::kDelete), "DELETE");
    EXPECT_EQ(sh::ToString(sh::HttpMethod::kGet), "GET");
    EXPECT_EQ(sh::ToString(sh::HttpMethod::kHead), "HEAD");
    EXPECT_EQ(sh::ToString(sh::HttpMethod::kPost), "POST");
    EXPECT_EQ(sh::ToString(sh::HttpMethod::kPut), "PUT");
    EXPECT_EQ(sh::ToString(sh::HttpMethod::kPatch), "PATCH");
    EXPECT_EQ(sh::ToString(sh::HttpMethod::kConnect), "CONNECT");
    EXPECT_EQ(sh::ToString(sh::HttpMethod::kOptions), "OPTIONS");
    EXPECT_EQ(sh::ToString(sh::HttpMethod::kTrace), "TRACE");
}

UTEST(ServerHttpMethodTest, FromString) {
    EXPECT_EQ(sh::HttpMethodFromString("DELETE"), sh::HttpMethod::kDelete);
    EXPECT_EQ(sh::HttpMethodFromString("GET"), sh::HttpMethod::kGet);
    EXPECT_EQ(sh::HttpMethodFromString("HEAD"), sh::HttpMethod::kHead);
    EXPECT_EQ(sh::HttpMethodFromString("POST"), sh::HttpMethod::kPost);
    EXPECT_EQ(sh::HttpMethodFromString("PUT"), sh::HttpMethod::kPut);
    EXPECT_EQ(sh::HttpMethodFromString("PATCH"), sh::HttpMethod::kPatch);
    EXPECT_EQ(sh::HttpMethodFromString("CONNECT"), sh::HttpMethod::kConnect);
    EXPECT_EQ(sh::HttpMethodFromString("OPTIONS"), sh::HttpMethod::kOptions);
    EXPECT_EQ(sh::HttpMethodFromString("TRACE"), sh::HttpMethod::kTrace);

    UEXPECT_THROW(sh::HttpMethodFromString("TRAC"), std::runtime_error);
    UEXPECT_THROW(sh::HttpMethodFromString("TRACER"), std::runtime_error);
    UEXPECT_THROW(sh::HttpMethodFromString("trace"), std::runtime_error);
}

// TRACE must be registrable in a handler `method:` list, unlike CONNECT, which
// userver never routes to a handler.
UTEST(ServerHttpMethodTest, IsHandlerMethod) {
    EXPECT_TRUE(sh::IsHandlerMethod(sh::HttpMethod::kGet));
    EXPECT_TRUE(sh::IsHandlerMethod(sh::HttpMethod::kOptions));
    EXPECT_TRUE(sh::IsHandlerMethod(sh::HttpMethod::kTrace));

    EXPECT_FALSE(sh::IsHandlerMethod(sh::HttpMethod::kConnect));
}

USERVER_NAMESPACE_END
