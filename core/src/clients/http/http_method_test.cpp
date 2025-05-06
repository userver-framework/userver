#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/http/request.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
namespace ch = clients::http;
}

UTEST(ClientHttpMethodTest, Convert) {
    EXPECT_EQ(ch::ToStringView(ch::HttpMethod::kGet), "GET");
    EXPECT_EQ(ch::ToStringView(ch::HttpMethod::kHead), "HEAD");
    EXPECT_EQ(ch::ToStringView(ch::HttpMethod::kPost), "POST");
    EXPECT_EQ(ch::ToStringView(ch::HttpMethod::kPatch), "PATCH");
    EXPECT_EQ(ch::ToStringView(ch::HttpMethod::kPut), "PUT");
    EXPECT_EQ(ch::ToStringView(ch::HttpMethod::kDelete), "DELETE");
    EXPECT_EQ(ch::ToStringView(ch::HttpMethod::kOptions), "OPTIONS");

    EXPECT_EQ(ch::HttpMethodFromString("GET"), ch::HttpMethod::kGet);
    EXPECT_EQ(ch::HttpMethodFromString("HEAD"), ch::HttpMethod::kHead);
    EXPECT_EQ(ch::HttpMethodFromString("POST"), ch::HttpMethod::kPost);
    EXPECT_EQ(ch::HttpMethodFromString("PATCH"), ch::HttpMethod::kPatch);
    EXPECT_EQ(ch::HttpMethodFromString("PUT"), ch::HttpMethod::kPut);
    EXPECT_EQ(ch::HttpMethodFromString("DELETE"), ch::HttpMethod::kDelete);
    EXPECT_EQ(ch::HttpMethodFromString("OPTIONS"), ch::HttpMethod::kOptions);

    UEXPECT_THROW(ch::HttpMethodFromString("123"), std::runtime_error);
}

USERVER_NAMESPACE_END
