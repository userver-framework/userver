#include <userver/chaotic/openapi/client/content_encoding.hpp>

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include <userver/chaotic/openapi/client/command_control.hpp>
#include <userver/clients/http/client.hpp>
#include <userver/compression/gzip.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/http_server_mock.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace co = chaotic::openapi::client;

UTEST(EncodeRequest, AutoIsNoop) {
    auto http_client_ptr = utest::CreateHttpClient();
    auto request = http_client_ptr->CreateRequest();
    request.url("http://localhost/").data("plain body");

    co::CommandControl cc;
    EXPECT_EQ(cc.encoding, co::CommandControl::ContentEncoding::kAuto);
    co::EncodeRequest(request, cc);

    EXPECT_EQ(request.GetData(), "plain body");
}

UTEST(EncodeRequest, GzipCompressesBodyAndSetsHeader) {
    const std::string original_body = "some request body to be gzip-compressed";

    bool called = false;
    const utest::HttpServerMock http_server([&](const utest::HttpServerMock::HttpRequest& request) {
        constexpr http::headers::PredefinedHeader kContentEncoding("Content-Encoding");
        called = true;

        EXPECT_EQ(request.headers.at(kContentEncoding), "gzip");
        EXPECT_EQ(compression::gzip::Decompress(request.body, original_body.size()), original_body);

        return utest::HttpServerMock::HttpResponse{};
    });

    auto http_client_ptr = utest::CreateHttpClient();
    auto request = http_client_ptr->CreateRequest();
    request.post(http_server.GetBaseUrl(), original_body);

    co::CommandControl cc;
    cc.encoding = co::CommandControl::ContentEncoding::kGzip;
    co::EncodeRequest(request, cc);

    // Body must be replaced with the gzip-compressed data right away
    EXPECT_NE(request.GetData(), original_body);
    EXPECT_EQ(compression::gzip::Decompress(request.GetData(), original_body.size()), original_body);

    auto response = request.perform();
    EXPECT_TRUE(called);
}

UTEST(EncodeRequest, GzipEmptyBodyIsNoop) {
    bool called = false;
    const utest::HttpServerMock http_server([&](const utest::HttpServerMock::HttpRequest& request) {
        constexpr http::headers::PredefinedHeader kContentEncoding("Content-Encoding");
        called = true;

        // No compression should happen for an empty body: no header, empty body
        EXPECT_EQ(request.headers.count(kContentEncoding), 0);
        EXPECT_TRUE(request.body.empty());

        return utest::HttpServerMock::HttpResponse{};
    });

    auto http_client_ptr = utest::CreateHttpClient();
    auto request = http_client_ptr->CreateRequest();
    request.post(http_server.GetBaseUrl(), std::string{});

    co::CommandControl cc;
    cc.encoding = co::CommandControl::ContentEncoding::kGzip;
    co::EncodeRequest(request, cc);

    EXPECT_TRUE(request.GetData().empty());

    auto response = request.perform();
    EXPECT_TRUE(called);
}

USERVER_NAMESPACE_END
