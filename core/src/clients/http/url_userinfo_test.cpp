#include <userver/clients/http/client_core.hpp>
#include <userver/clients/http/client_with_middlewares.hpp>
#include <userver/clients/http/error.hpp>
#include <userver/clients/http/form.hpp>
#include <userver/clients/http/middlewares/base.hpp>
#include <userver/clients/http/request.hpp>
#include <userver/clients/http/response.hpp>
#include <userver/clients/http/streamed_response.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/utils/impl/internal_tag.hpp>

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include <userver/utest/http_client.hpp>
#include <userver/utest/simple_server.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kUserinfo[] = {"user", "user:password", ":password", "", ":", "us%65r:p%40ss"};
constexpr std::string_view kSuccessResponse = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
constexpr std::string_view kBasicAuthorization = "Authorization: Basic dXNlcjpwYXNzd29yZA==";
constexpr auto kUrlWithUserinfo = "http://user:password@localhost/";

using Request = clients::http::Request;
using Server = utest::SimpleServer;

std::string WithUserinfo(const std::string& url, std::string_view userinfo) {
    const auto authority = url.find("://") + std::string_view{"://"}.size();
    return fmt::format("{}{}@{}", url.substr(0, authority), userinfo, url.substr(authority));
}

clients::http::ClientWithMiddlewares MakeClient(std::shared_ptr<clients::http::ClientCore> core, bool allowed) {
    return {
        utils::impl::InternalTag{},
        std::move(core),
        {},
        allowed ? clients::http::ClientOptions::kAllowUserInfo : clients::http::ClientOptions::kNone
    };
}

Server::Response Success(const Server::Request&) { return {std::string{kSuccessResponse}}; }

}  // namespace

UTEST(HttpClientUrlUserinfo, RejectsAllRequestUrlApis) {
    auto client = MakeClient(utest::impl::CreateHttpClientCore(), false);
    using SetUrl = std::function<void(Request&, const std::string&)>;
    const SetUrl setters[] = {
        [](Request& request, const auto& url) { request.url(url); },
        [](Request& request, const auto& url) { request.get(url); },
        [](Request& request, const auto& url) { request.head(url); },
        [](Request& request, const auto& url) { request.post(url, std::string{}); },
        [](Request& request, const auto& url) { request.post(url, clients::http::Form{}); },
        [](Request& request, const auto& url) { request.put(url); },
        [](Request& request, const auto& url) { request.patch(url); },
        [](Request& request, const auto& url) { request.delete_method(url); },
        [](Request& request, const auto& url) { request.delete_method(url, std::string{}); },
        [](Request& request, const auto& url) { std::move(request).url(url); },
        [](Request& request, const auto& url) { std::move(request).get(url); },
        [](Request& request, const auto& url) { std::move(request).head(url); },
        [](Request& request, const auto& url) { std::move(request).post(url, std::string{}); },
        [](Request& request, const auto& url) { std::move(request).post(url, clients::http::Form{}); },
        [](Request& request, const auto& url) { std::move(request).put(url); },
        [](Request& request, const auto& url) { std::move(request).patch(url); },
        [](Request& request, const auto& url) { std::move(request).delete_method(url); },
        [](Request& request, const auto& url) { std::move(request).delete_method(url, std::string{}); },
    };

    for (const auto scheme : {"http", "https", "ws", "wss"}) {
        for (const auto slashes : {"/", "//", "///"}) {
            for (const auto userinfo : kUserinfo) {
                const auto url = fmt::format("{}:{}{}@localhost/", scheme, slashes, userinfo);
                SCOPED_TRACE(url);
                for (const auto& setter : setters) {
                    auto request = client.CreateRequest();
                    UEXPECT_THROW(setter(request, url), clients::http::BadArgumentException);
                }
            }
        }
    }
}

UTEST(HttpClientUrlUserinfo, AllowsAtInPathQueryAndFragment) {
    auto client = MakeClient(utest::impl::CreateHttpClientCore(), false);
    const Server server{Success};
    const auto response =
        client.CreateRequest()
            .get(server.GetBaseUrl() + "/user:password@path?user:password@query#user:password@fragment")
            .timeout(utest::kMaxTestWaitTime)
            .perform();
    EXPECT_TRUE(response->IsOk());
}

UTEST(HttpClientUrlUserinfo, RejectsRedirectsBeforeConnecting) {
    auto client = MakeClient(utest::impl::CreateHttpClientCore(), false);
    const Server destination{Success};
    for (const auto userinfo : kUserinfo) {
        for (const auto prefix : {"http:/", "http://", "http:///", "//", "\t//", "\v//", "\f//"}) {
            const auto authority = destination.GetBaseUrl().substr(std::string_view{"http://"}.size());
            const auto location = fmt::format("{}{}@{}", prefix, userinfo, authority);
            SCOPED_TRACE(location);
            const Server redirect{[&](const auto&) -> Server::Response {
                return {fmt::format("HTTP/1.1 302 Found\r\nLocation: {}\r\nContent-Length: 0\r\n\r\n", location)};
            }};
            auto request = client.CreateRequest().get(redirect.GetBaseUrl()).timeout(utest::kMaxTestWaitTime);
            UEXPECT_THROW((void)request.perform(), clients::http::AuthFailedException);
        }
    }
    EXPECT_EQ(destination.GetConnectionsOpenedCount(), 0);
}

UTEST(HttpClientUrlUserinfo, ReusesRequestAfterRejectedRedirect) {
    auto client = MakeClient(utest::impl::CreateHttpClientCore(), false);
    const Server destination{Success};
    const auto location = WithUserinfo(destination.GetBaseUrl(), "");
    const Server server{[&](const auto& request) -> Server::Response {
        if (request.starts_with("GET /reject ")) {
            return {
                fmt::format("HTTP/1.1 302 Found\r\nLocation:\r\nLocation: {}\r\nContent-Length: 0\r\n\r\n", location)
            };
        }
        return {std::string{kSuccessResponse}};
    }};
    auto request = client.CreateRequest().get(server.GetBaseUrl() + "/reject").timeout(utest::kMaxTestWaitTime);
    UEXPECT_THROW((void)request.perform(), clients::http::AuthFailedException);
    EXPECT_TRUE(request.get(server.GetBaseUrl() + "/valid").perform()->IsOk());
    EXPECT_EQ(destination.GetConnectionsOpenedCount(), 0);
}

UTEST(HttpClientUrlUserinfo, AllowsAtInRedirectPathQueryAndFragment) {
    auto client = MakeClient(utest::impl::CreateHttpClientCore(), false);
    const Server destination{Success};
    constexpr std::string_view kPath = "/user:password@path?user:password@query#user:password@fragment";
    for (const bool absolute : {false, true}) {
        const auto location = fmt::format("{}{}", absolute ? destination.GetBaseUrl() : "", kPath);
        const Server redirect{[&](const auto& request) -> Server::Response {
            if (request.starts_with("GET /user:password@path?")) {
                return {std::string{kSuccessResponse}};
            }
            return {fmt::format("HTTP/1.1 302 Found\r\nLocation: {}\r\nContent-Length: 0\r\n\r\n", location)};
        }};
        EXPECT_TRUE(client.CreateRequest().get(redirect.GetBaseUrl()).timeout(utest::kMaxTestWaitTime).perform()->IsOk()
        );
    }
}

UTEST(HttpClientUrlUserinfo, KeepsUnfollowedLocations) {
    auto client = MakeClient(utest::impl::CreateHttpClientCore(), false);
    const Server destination{Success};
    const auto location = WithUserinfo(destination.GetBaseUrl(), ":password");
    for (const bool follow : {false, true}) {
        const auto status = follow ? "200 OK" : "302 Found";
        const Server server{[&](const auto&) -> Server::Response {
            return {fmt::format("HTTP/1.1 {}\r\nLocation: {}\r\nContent-Length: 0\r\n\r\n", status, location)};
        }};
        EXPECT_EQ(
            client.CreateRequest()
                .get(server.GetBaseUrl())
                .follow_redirects(follow)
                .timeout(utest::kMaxTestWaitTime)
                .perform()
                ->status_code(),
            follow ? clients::http::Status::kOk : clients::http::Status::kFound
        );
    }
    EXPECT_EQ(destination.GetConnectionsOpenedCount(), 0);
}

UTEST(HttpClientUrlUserinfo, AllowsExplicitOptInAndIsolatesSharedCore) {
    const auto core = utest::impl::CreateHttpClientCore();
    clients::http::ClientWithMiddlewares restricted{utils::impl::InternalTag{}, core, {}};
    auto unrestricted = MakeClient(core, true);
    const Server server{[](const auto& request) -> Server::Response {
        EXPECT_NE(request.find(kBasicAuthorization), std::string::npos);
        return {std::string{kSuccessResponse}};
    }};
    const auto url = WithUserinfo(server.GetBaseUrl(), "user:password");
    for (const auto iteration : {0, 1}) {
        SCOPED_TRACE(iteration);
        EXPECT_TRUE(unrestricted.CreateRequest().get(url).timeout(utest::kMaxTestWaitTime).perform()->IsOk());
        UEXPECT_THROW(restricted.CreateRequest().get(url), clients::http::BadArgumentException);
    }
    const Server redirect_destination{Success};
    const auto redirect_url = WithUserinfo(redirect_destination.GetBaseUrl(), "user:password");
    const Server redirect{[&](const auto&) -> Server::Response {
        return {fmt::format("HTTP/1.1 302 Found\r\nLocation: {}\r\nContent-Length: 0\r\n\r\n", redirect_url)};
    }};
    EXPECT_TRUE(
        unrestricted.CreateRequest().get(redirect.GetBaseUrl()).timeout(utest::kMaxTestWaitTime).perform()->IsOk()
    );
    EXPECT_EQ(redirect_destination.GetConnectionsOpenedCount(), 1);
}

UTEST(HttpClientUrlUserinfo, KeepsExplicitAuthentication) {
    auto client = MakeClient(utest::impl::CreateHttpClientCore(), false);
    const Server server{[](const auto& request) -> Server::Response {
        EXPECT_NE(request.find(kBasicAuthorization), std::string::npos);
        return {std::string{kSuccessResponse}};
    }};
    EXPECT_TRUE(client.CreateRequest()
                    .get(server.GetBaseUrl())
                    .http_auth_type(clients::http::HttpAuthType::kBasic, false, "user", "password")
                    .timeout(utest::kMaxTestWaitTime)
                    .perform()
                    ->IsOk());
}

UTEST(HttpClientUrlUserinfo, KeepsExplicitAuthenticationOnRelativeRedirects) {
    auto client = MakeClient(utest::impl::CreateHttpClientCore(), false);
    for (const auto location : {"/done", "./done", "?done"}) {
        SCOPED_TRACE(location);
        const Server server{[&](const auto& request) -> Server::Response {
            EXPECT_NE(request.find(kBasicAuthorization), std::string::npos);
            if (request.starts_with("GET /start ")) {
                return {fmt::format("HTTP/1.1 302 Found\r\nLocation: {}\r\nContent-Length: 0\r\n\r\n", location)};
            }
            return {std::string{kSuccessResponse}};
        }};
        auto request =
            client.CreateRequest()
                .get(server.GetBaseUrl() + "/start")
                .http_auth_type(clients::http::HttpAuthType::kBasic, false, "user", "password")
                .timeout(utest::kMaxTestWaitTime);
        EXPECT_TRUE(request.perform()->IsOk());
        EXPECT_EQ(server.GetConnectionsOpenedCount(), 2);
    }
}

UTEST(HttpClientUrlUserinfo, RejectsStreamingRedirectsBeforeConnecting) {
    auto client = MakeClient(utest::impl::CreateHttpClientCore(), false);
    const Server destination{Success};
    for (const auto userinfo : kUserinfo) {
        const auto location = WithUserinfo(destination.GetBaseUrl(), userinfo);
        SCOPED_TRACE(location);
        const Server redirect{[&](const auto&) -> Server::Response {
            return {fmt::format("HTTP/1.1 302 Found\r\nLocation: {}\r\nContent-Length: 0\r\n\r\n", location)};
        }};
        auto request = client.CreateRequest().get(redirect.GetBaseUrl()).timeout(utest::kMaxTestWaitTime);
        auto stream = request.async_perform_stream_body(concurrent::StringStreamQueue::Create());
        UEXPECT_THROW((void)stream.StatusCode(), clients::http::AuthFailedException);
    }
    EXPECT_EQ(destination.GetConnectionsOpenedCount(), 0);
}

UTEST(HttpClientUrlUserinfo, KeepsProxyAuthentication) {
    auto client = MakeClient(utest::impl::CreateHttpClientCore(), false);
    const Server proxy{[](const auto& request) -> Server::Response {
        EXPECT_NE(request.find("Proxy-Authorization: Basic dXNlcjpwYXNzd29yZA=="), std::string::npos);
        return {std::string{kSuccessResponse}};
    }};
    const auto proxy_url = WithUserinfo(proxy.GetBaseUrl(), "user:password");
    EXPECT_TRUE(client.CreateRequest()
                    .get("http://destination.invalid/")
                    .proxy(proxy_url)
                    .timeout(utest::kMaxTestWaitTime)
                    .perform()
                    ->IsOk());
}

UTEST(HttpClientUrlUserinfo, PublicRequestOption) {
    const auto client = utest::CreateHttpClient();
    auto request = client->CreateRequest();
    UEXPECT_THROW(request.url(kUrlWithUserinfo), clients::http::BadArgumentException);

    request.SetUrlUserinfoAllowed(true);
    UEXPECT_NO_THROW(request.url(kUrlWithUserinfo));
    UEXPECT_THROW(request.SetUrlUserinfoAllowed(false), clients::http::BadArgumentException);
    UEXPECT_THROW(request.url(kUrlWithUserinfo), clients::http::BadArgumentException);

    const Server server{Success};
    request.url(server.GetBaseUrl());
    request.SetUrlUserinfoAllowed(false);
    EXPECT_TRUE(request.timeout(utest::kMaxTestWaitTime).perform()->IsOk());
}

UTEST(HttpClientUrlUserinfo, DefaultsToRejectingUserinfoOnFreshAndReusedCoreRequests) {
    const auto core = utest::impl::CreateHttpClientCore();
    const Server server{Success};
    const auto url = WithUserinfo(server.GetBaseUrl(), "user:password");
    {
        auto request = core->CreateRequest();
        UEXPECT_THROW(request.url(url), clients::http::BadArgumentException);
        request.SetUrlUserinfoAllowed(true);
        EXPECT_TRUE(request.get(url).timeout(utest::kMaxTestWaitTime).perform()->IsOk());
    }
    UEXPECT_THROW(core->CreateRequest().get(url), clients::http::BadArgumentException);
}

USERVER_NAMESPACE_END
