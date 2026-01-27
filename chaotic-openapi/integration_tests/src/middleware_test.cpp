#include <userver/clients/http/client.hpp>
#include <userver/dynamic_config/test_helpers.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/http_server_mock.hpp>
#include <userver/utest/utest.hpp>

#include <userver/chaotic/openapi/middlewares/follow_redirects_middleware.hpp>
#include <userver/chaotic/openapi/middlewares/logging_middleware.hpp>
#include <userver/chaotic/openapi/middlewares/proxy_middleware.hpp>
#include <userver/chaotic/openapi/middlewares/qos_middleware.hpp>
#include <userver/chaotic/openapi/middlewares/ssl_middleware.hpp>

#include <string>
#include <userver/logging/log.hpp>
#include <userver/utest/default_logger_fixture.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
constexpr http::headers::PredefinedHeader kLocation("Location");

}

UTEST(TestMiddleware, FollowRedirectsMiddleware) {
    bool redirect_visited = false;

    const utest::HttpServerMock http_server([&redirect_visited](const utest::HttpServerMock::HttpRequest& request) {
        if (request.path == "/test") {
            auto response = utest::HttpServerMock::HttpResponse{302};
            response.headers[kLocation] = "/redirected";
            return response;
        }

        if (request.path == "/redirected") {
            redirect_visited = true;
            auto response = utest::HttpServerMock::HttpResponse{200};
            response.body = R"({"status":"ok"})";
            return response;
        }

        return utest::HttpServerMock::HttpResponse{404};
    });

    auto http_client_ptr = utest::CreateHttpClient();
    auto request = http_client_ptr->CreateRequest();

    request.url(http_server.GetBaseUrl() + "/test");

    chaotic::openapi::FollowRedirectsMiddleware follow_redirects_middleware(true);
    follow_redirects_middleware.OnRequest(request);

    auto response = request.perform();

    EXPECT_TRUE(redirect_visited);
    EXPECT_EQ(response->status_code(), 200);
    EXPECT_EQ(response->body(), R"({"status":"ok"})");
}

UTEST(TestMiddleware, ProxyMiddleware) {
    const utest::HttpServerMock destination_server([](const utest::HttpServerMock::HttpRequest& request) {
        EXPECT_EQ(request.path, "/test");

        auto response = utest::HttpServerMock::HttpResponse{200};
        response.body = R"({"status":"ok","server":"destination"})";
        return response;
    });

    bool proxy_called = false;
    const utest::HttpServerMock proxy_server([&proxy_called](const utest::HttpServerMock::HttpRequest& /*request*/) {
        proxy_called = true;

        auto response = utest::HttpServerMock::HttpResponse{200};
        response.body = R"({"status":"ok","server":"proxy"})";
        return response;
    });

    auto http_client_ptr = utest::CreateHttpClient();
    auto request = http_client_ptr->CreateRequest();

    request.url(destination_server.GetBaseUrl() + "/test");

    std::string proxy_url = proxy_server.GetBaseUrl();
    chaotic::openapi::ProxyMiddleware proxy_middleware(std::move(proxy_url));

    proxy_middleware.OnRequest(request);

    auto response = request.perform();

    EXPECT_EQ(response->status_code(), 200);
    EXPECT_EQ(response->body(), R"({"status":"ok","server":"proxy"})");
}

UTEST(TestMiddleware, SslMiddleware) {
    constexpr const std::string_view kCertPem = R"(-----BEGIN CERTIFICATE-----
MIIDHTCCAgWgAwIBAgIUdeC/2p5YeWqKXWWfIWXVidqA2dUwDQYJKoZIhvcNAQEL
BQAwHjEcMBoGA1UEAwwTdXNlcnZlci1jcnlwdG8tdGVzdDAeFw0yMzEyMDEwMDAw
MDBaFw00MzA4MTgwMDAwMDBaMB4xHDAaBgNVBAMME3VzZXJ2ZXItY3J5cHRvLXRl
c3QwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCjTeIOQqFQ5WXe3epA
xyDFUN11yQFu4Bz6DmFkPjuNjBxMHVeZ2dTHpjFG5sSokQUG+OSwJ3M98jlXyvgQ
bDONhzGvORH55e8I3+OuYQN/i7HcUUDhIWc/OmnyO2/DUI/E2/uLANCDC5cBgjpe
ysxiAxaJZoRBK0Gq0VIubJbAVRrtgMgby2J0JLFYyPnPa3rIElJwNZEX5a2+PnwO
zeRqWTGTIUPVaxMrxu9Ti/1lGAUg9DCzbg36VALIbnnMhc1CINmGpNAXv611M1tv
bSrk8qpXW6HY7N/ezX/TW4UIfHpBXGuR1zSkFqGEJ/cgiOigdNtyjjwobIvRrO40
zhP5AgMBAAGjUzBRMB0GA1UdDgQWBBSw8zB5AvB60Pq3ayUe9Unoaa3gujAfBgNV
HSMEGDAWgBSw8zB5AvB60Pq3ayUe9Unoaa3gujAPBgNVHRMBAf8EBTADAQH/MA0G
CSqGSIb3DQEBCwUAA4IBAQCZPthv9VMkK+c7bjqo1PAMT8NAUSvAGnt97eKBl4E0
tJVhFQWe52QkIxZLhTg6KgBZa5JxoL3Lgat2oT+WH15ebghp+uzjSs+j/XWESrme
BQaTdWpi66RnB0sFnZ5KkDXKoLwz2eLY53p8rDuDrukGAhu9rKsmPlINgRICjSL6
AKe1Kl8BJ6XLnxfHps7gutUGcSatpKP0vaN3BnYEnNeQ4jDqTOeRgujGoDYCkAoX
vnt5k03sADG1HQMJJ+okTNhM3X0nbmxSxQw3arVzkTtkY39zGPqQxKgDch2uCzEv
iW5OwYvGErHvYQaO0LtwjzO8LamystYgUIXVV+fFL3w6
-----END CERTIFICATE-----)";

    const utest::HttpServerMock http_server([](const utest::HttpServerMock::HttpRequest& request) {
        EXPECT_EQ(request.path, "/test");

        auto response = utest::HttpServerMock::HttpResponse{200};
        response.body = R"({"status":"ok"})";
        return response;
    });

    auto http_client_ptr = utest::CreateHttpClient();
    auto request = http_client_ptr->CreateRequest();

    request.url(http_server.GetBaseUrl() + "/test");

    crypto::Certificate cert;
    try {
        cert = crypto::Certificate::LoadFromString(kCertPem);
        EXPECT_TRUE(cert.GetPemString().has_value());
    } catch (const std::exception& e) {
        FAIL() << "Failed to load certificate: " << e.what();
    }

    chaotic::openapi::SslMiddleware ssl_middleware(std::move(cert));
    ssl_middleware.OnRequest(request);

    auto response = request.perform();

    EXPECT_EQ(response->status_code(), 200);
    EXPECT_EQ(response->body(), R"({"status":"ok"})");
}

USERVER_NAMESPACE_END
