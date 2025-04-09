#include <userver/utest/utest.hpp>

#include <userver/clients/http/client.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/http_server_mock.hpp>

#include <client/test_object/responses.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

UTEST(Responses, Status200) {
    utest::HttpServerMock http_server([](const utest::HttpServerMock::HttpRequest&) {
        utest::HttpServerMock::HttpResponse r;
        r.response_status = 200;
        r.body = R"({"bar": "bar"})";
        return r;
    });
    auto http_client = utest::CreateHttpClient();
    auto http_response = http_client->CreateRequest().get(http_server.GetBaseUrl() + "/test1").perform();

    auto response = ::clients::test_object::test1_post::ParseResponse(*http_response);
    auto response200 = std::get<::clients::test_object::test1_post::Response200>(response);
    EXPECT_EQ(response200.body.bar, "bar");
}

UTEST(Responses, Status500) {
    utest::HttpServerMock http_server([](const utest::HttpServerMock::HttpRequest&) {
        utest::HttpServerMock::HttpResponse r;
        r.response_status = 500;
        return r;
    });
    auto http_client = utest::CreateHttpClient();
    auto http_response = http_client->CreateRequest().get(http_server.GetBaseUrl() + "/test1").perform();

    UEXPECT_THROW(
        ::clients::test_object::test1_post::ParseResponse(*http_response),
        ::clients::test_object::test1_post::Response500
    );
}

}  // namespace

USERVER_NAMESPACE_END
