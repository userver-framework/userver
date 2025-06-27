#include <userver/utest/utest.hpp>

#include <userver/clients/http/client.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/http_server_mock.hpp>

#include <clients/multiple_content_types/responses.hpp>
#include <clients/response_headers/responses.hpp>
#include <clients/test_object/client_impl.hpp>
#include <clients/test_object/responses.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

UTEST(Responses, Smoke) {
    const utest::HttpServerMock http_server([](const utest::HttpServerMock::HttpRequest&) {
        utest::HttpServerMock::HttpResponse r;
        r.response_status = 200;
        r.body = R"({"bar": "bar"})";
        return r;
    });
    auto http_client = utest::CreateHttpClient();
    ::clients::test_object::ClientImpl client(
        {
            http_server.GetBaseUrl(),
        },
        *http_client
    );

    auto response = client.Test1Post({}, {});
    auto response200 = std::get<::clients::test_object::test1::post::Response200>(response);
    EXPECT_EQ(response200.body.bar, "bar");
}

UTEST(Responses, Status200) {
    const utest::HttpServerMock http_server([](const utest::HttpServerMock::HttpRequest&) {
        utest::HttpServerMock::HttpResponse r;
        r.response_status = 200;
        r.body = R"({"bar": "bar"})";
        return r;
    });
    auto http_client = utest::CreateHttpClient();
    auto http_response = http_client->CreateRequest().get(http_server.GetBaseUrl() + "/test1").perform();

    auto response = ::clients::test_object::test1::post::ParseResponse(*http_response);
    auto response200 = std::get<::clients::test_object::test1::post::Response200>(response);
    EXPECT_EQ(response200.body.bar, "bar");
}

UTEST(Responses, Status500) {
    const utest::HttpServerMock http_server([](const utest::HttpServerMock::HttpRequest&) {
        utest::HttpServerMock::HttpResponse r;
        r.response_status = 500;
        return r;
    });
    auto http_client = utest::CreateHttpClient();
    auto http_response = http_client->CreateRequest().get(http_server.GetBaseUrl() + "/test1").perform();

    UEXPECT_THROW(
        ::clients::test_object::test1::post::ParseResponse(*http_response),
        ::clients::test_object::test1::post::Response500
    );
}

UTEST(Responses, StatusUnknown) {
    const utest::HttpServerMock http_server([](const utest::HttpServerMock::HttpRequest&) {
        utest::HttpServerMock::HttpResponse r;
        r.response_status = 555;
        return r;
    });
    auto http_client = utest::CreateHttpClient();
    auto http_response = http_client->CreateRequest().get(http_server.GetBaseUrl() + "/test1").perform();

    try {
        [[maybe_unused]] auto response = ::clients::test_object::test1::post::ParseResponse(*http_response);
        FAIL();
    } catch (const ::clients::test_object::test1::post::ExceptionWithStatusCode& exc) {
        EXPECT_EQ(exc.GetStatusCode(), 555);
        EXPECT_EQ(std::string(exc.what()), "POST /test1");
    }
}

UTEST(Responses, Timeout) {
    const utest::HttpServerMock http_server([](const utest::HttpServerMock::HttpRequest&) {
        utest::HttpServerMock::HttpResponse r;
        engine::SleepFor(std::chrono::milliseconds(100));
        r.response_status = 200;
        return r;
    });

    auto http_client = utest::CreateHttpClient();
    ::clients::test_object::ClientImpl client(
        {
            http_server.GetBaseUrl(),
        },
        *http_client
    );

    UEXPECT_THROW(client.Test1Post({}, {}), ::clients::test_object::test1::post::TimeoutException);
}

UTEST(ResponsesMultipleContentType, ApplicationJson) {
    const utest::HttpServerMock http_server([](const utest::HttpServerMock::HttpRequest&) {
        utest::HttpServerMock::HttpResponse r;
        r.response_status = 200;
        r.body = R"({"bar": "a"})";
        r.headers[std::string{"Content-Type"}] = "application/json";
        return r;
    });
    auto http_client = utest::CreateHttpClient();
    auto http_response = http_client->CreateRequest().get(http_server.GetBaseUrl() + "/test1").perform();

    namespace client = ::clients::multiple_content_types::test1::post;
    EXPECT_EQ(
        std::get<client::Response200BodyApplicationJson>(client::ParseResponse(*http_response).body),
        (client::Response200BodyApplicationJson{"a"})
    );
}

UTEST(ResponsesMultipleContentType, ApplicationOctetStream) {
    const utest::HttpServerMock http_server([](const utest::HttpServerMock::HttpRequest&) {
        utest::HttpServerMock::HttpResponse r;
        r.response_status = 200;
        r.body = "blabla";
        r.headers[std::string{"Content-Type"}] = "application/octet-stream";
        return r;
    });
    auto http_client = utest::CreateHttpClient();
    auto http_response = http_client->CreateRequest().get(http_server.GetBaseUrl() + "/test1").perform();

    namespace client = ::clients::multiple_content_types::test1::post;
    EXPECT_EQ(
        std::get<client::Response200ApplicationOctetStream>(client::ParseResponse(*http_response).body),
        (client::Response200ApplicationOctetStream{"blabla"})
    );
}

UTEST(ResponsesMultipleContentType, UnknownContentType) {
    const utest::HttpServerMock http_server([](const utest::HttpServerMock::HttpRequest&) {
        utest::HttpServerMock::HttpResponse r;
        r.response_status = 200;
        r.body = R"({"bar": "a"})";
        r.headers[std::string{"Content-Type"}] = "application/unknown";
        return r;
    });
    auto http_client = utest::CreateHttpClient();
    auto http_response = http_client->CreateRequest().get(http_server.GetBaseUrl() + "/test1").perform();

    namespace client = ::clients::multiple_content_types::test1::post;
    UEXPECT_THROW(client::ParseResponse(*http_response), client::ExceptionWithStatusCode);
}

UTEST(ResponsesMultipleContentType, InvalidJson) {
    const utest::HttpServerMock http_server([](const utest::HttpServerMock::HttpRequest&) {
        utest::HttpServerMock::HttpResponse r;
        r.response_status = 200;
        r.body = R"({"bar": "a")";
        r.headers[std::string{"Content-Type"}] = "application/json";
        return r;
    });
    auto http_client = utest::CreateHttpClient();
    auto http_response = http_client->CreateRequest().get(http_server.GetBaseUrl() + "/test1").perform();

    namespace client = ::clients::multiple_content_types::test1::post;
    UEXPECT_THROW(client::ParseResponse(*http_response), client::ExceptionWithStatusCode);
}

UTEST(ResponsesMultipleContentType, InvalidSchema) {
    const utest::HttpServerMock http_server([](const utest::HttpServerMock::HttpRequest&) {
        utest::HttpServerMock::HttpResponse r;
        r.response_status = 200;
        r.body = R"({"barrrrrr": "a"})";
        r.headers[std::string{"Content-Type"}] = "application/json";
        return r;
    });
    auto http_client = utest::CreateHttpClient();
    auto http_response = http_client->CreateRequest().get(http_server.GetBaseUrl() + "/test1").perform();

    namespace client = ::clients::multiple_content_types::test1::post;
    UEXPECT_THROW(client::ParseResponse(*http_response), client::ExceptionWithStatusCode);
}

UTEST(ResponsesMultipleContentType, HeaderParse) {
    const utest::HttpServerMock http_server([](const utest::HttpServerMock::HttpRequest&) {
        utest::HttpServerMock::HttpResponse r;
        r.response_status = 200;
        r.body = R"({})";
        r.headers[std::string{"Content-Type"}] = "application/json";
        r.headers[std::string{"X-Header"}] = "string";
        return r;
    });
    auto http_client = utest::CreateHttpClient();
    auto http_response = http_client->CreateRequest().get(http_server.GetBaseUrl() + "/test1").perform();

    namespace client = ::clients::response_headers::test1::post;
    auto response = client::ParseResponse(*http_response);
    EXPECT_EQ(response.X_Header, "string");
}

}  // namespace

USERVER_NAMESPACE_END
