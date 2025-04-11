#include <userver/utest/utest.hpp>

#include <userver/clients/http/client.hpp>
#include <userver/http/content_type.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/http_server_mock.hpp>

#include <client/multiple_content_types/requests.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace client = ::clients::multiple_content_types::test1_post;

UTEST(RequestsMultipleContentTypes, Json) {
    utest::HttpServerMock http_server([&](const utest::HttpServerMock::HttpRequest& request) {
        EXPECT_EQ(request.body, R"({"foo":"a"})");
        EXPECT_EQ(request.headers.at(std::string{"Content-Type"}), "application/json");
        return utest::HttpServerMock::HttpResponse{200};
    });

    auto http_client_ptr = utest::CreateHttpClient();
    auto request = http_client_ptr->CreateRequest();

    request.url(http_server.GetBaseUrl());
    client::SerializeRequest({client::RequestBodyApplicationJson{"a"}}, request);

    auto response = request.perform();
    EXPECT_EQ(response->status_code(), 200);
}

UTEST(RequestsMultipleContentTypes, XWwwFormUrlencoded) {
    utest::HttpServerMock http_server([&](const utest::HttpServerMock::HttpRequest& request) {
        EXPECT_EQ(request.body, "is_smoking=true&salary=1000.500000&age=30&password=123%20456&name=abc");
        EXPECT_EQ(request.headers.at(std::string{"Content-Type"}), "application/x-www-form-urlencoded");
        return utest::HttpServerMock::HttpResponse{200};
    });

    auto http_client_ptr = utest::CreateHttpClient();
    auto request = http_client_ptr->CreateRequest();

    request.url(http_server.GetBaseUrl());
    client::SerializeRequest(
        {client::RequestBodyApplicationXWwwFormUrlencoded{"abc", "123 456", 30, 1000.5, true}}, request
    );

    auto response = request.perform();
    EXPECT_EQ(response->status_code(), 200);
}

UTEST(RequestsMultipleContentTypes, MultipartFormData) {
    utest::HttpServerMock http_server([&](const utest::HttpServerMock::HttpRequest& request) {
        http::ContentType content_type(request.headers.at(std::string{"Content-Type"}));
        EXPECT_EQ(content_type.MediaType(), "multipart/form-data");
        const auto& boundary = content_type.Boundary();
        EXPECT_EQ(
            request.body,
            "--" + boundary +
                "\r\n"
                "Content-Disposition: form-data; name=\"filename\"\r\n"
                "\r\nfilename\r\n" +
                "--" + boundary +
                "\r\n"
                "Content-Disposition: form-data; name=\"content\"\r\n"
                "\r\nfile\ncontent\r\n" +
                "--" + boundary + "--\r\n"
        );
        return utest::HttpServerMock::HttpResponse{200};
    });

    auto http_client_ptr = utest::CreateHttpClient();
    auto request = http_client_ptr->CreateRequest();

    request.url(http_server.GetBaseUrl());
    client::SerializeRequest({client::RequestBodyMultipartFormData{"filename", "file\ncontent"}}, request);

    auto response = request.perform();
    EXPECT_EQ(response->status_code(), 200);
}

UTEST(RequestsMultipleContentTypes, OctetStream) {
    utest::HttpServerMock http_server([&](const utest::HttpServerMock::HttpRequest& request) {
        EXPECT_EQ(request.body, "blabla");
        EXPECT_EQ(request.headers.at(std::string{"Content-Type"}), "application/octet-stream");
        return utest::HttpServerMock::HttpResponse{200};
    });

    auto http_client_ptr = utest::CreateHttpClient();
    auto request = http_client_ptr->CreateRequest();

    request.url(http_server.GetBaseUrl());
    client::SerializeRequest({client::RequestBodyApplicationOctetStream{"blabla"}}, request);

    auto response = request.perform();
    EXPECT_EQ(response->status_code(), 200);
}
}  // namespace

USERVER_NAMESPACE_END
