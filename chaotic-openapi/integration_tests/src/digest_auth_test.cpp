#include <userver/utest/utest.hpp>

#include <cstdint>
#include <optional>
#include <string>

#include <fmt/format.h>

#include <userver/crypto/hash.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/http_server_mock.hpp>
#include <userver/utils/from_string.hpp>

#include <clients/digest_auth/client_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace client_ns = ::clients::digest_auth;

// Implements a minimal MD5 Digest-auth challenge/response server for
// testing purposes. The nonce is kept fixed so the nc counter can be tracked
// across calls; the mock validates the Authorization header on every request.
struct DigestServerState {
    static constexpr std::string_view kRealm = "testrealm";
    static constexpr std::string_view kNonce = "testnonce123";
    static constexpr std::string_view kQop = "auth";

    std::string username;
    std::string password;

    // nc value observed in the last authenticated request (0 = none yet).
    std::uint32_t last_nc{0};
    bool last_auth_valid{false};

    std::string ha1() const { return crypto::hash::weak::Md5(fmt::format("{}:{}:{}", username, kRealm, password)); }

    static std::string ha2(std::string_view method, std::string_view uri) {
        return crypto::hash::weak::Md5(fmt::format("{}:{}", method, uri));
    }

    // Extracts a named field from a Digest Authorization header value.
    // Handles both quoted ("value") and unquoted (value) forms.
    static std::optional<std::string> ExtractDigestField(const std::string& header, const std::string& field) {
        const std::string needle = field + "=";
        const auto pos = header.find(needle);
        if (pos == std::string::npos) {
            return std::nullopt;
        }
        const auto start = pos + needle.size();
        if (start >= header.size()) {
            return std::nullopt;
        }
        if (header[start] == '"') {
            const auto end = header.find('"', start + 1);
            if (end == std::string::npos) {
                return std::nullopt;
            }
            return header.substr(start + 1, end - start - 1);
        }
        auto end = start;
        while (end < header.size() && header[end] != ',' && header[end] != ' ' && header[end] != '\t') {
            ++end;
        }
        return header.substr(start, end - start);
    }

    // Validates the Authorization header. Returns true and updates last_nc on
    // success; returns false if the header is absent or the digest is wrong.
    bool ValidateAuth(const utest::HttpServerMock::HttpRequest& request) {
        const auto it = request.headers.find(http::headers::kAuthorization);
        if (it == request.headers.end()) {
            return false;
        }
        const std::string& auth_header = it->second;

        const auto nc_str = ExtractDigestField(auth_header, "nc");
        const auto cnonce = ExtractDigestField(auth_header, "cnonce");
        const auto resp = ExtractDigestField(auth_header, "response");
        const auto uri_field = ExtractDigestField(auth_header, "uri");

        if (!nc_str || !cnonce || !resp || !uri_field) {
            return false;
        }

        const auto nc_val = utils::FromHexString(*nc_str);

        const std::string expected_response = crypto::hash::weak::Md5(fmt::format(
            "{}:{}:{}:{}:{}:{}",
            ha1(),
            kNonce,
            *nc_str,
            *cnonce,
            kQop,
            ha2(clients::http::ToStringView(request.method), *uri_field)
        ));

        if (expected_response != *resp) {
            return false;
        }

        last_nc = nc_val;
        last_auth_valid = true;
        return true;
    }
};

utest::HttpServerMock::HttpResponse MakeDigestChallenge() {
    utest::HttpServerMock::HttpResponse response;
    response.response_status = 401;
    response.headers[http::headers::kWWWAuthenticate] = fmt::format(
        R"(Digest realm="{}", nonce="{}", qop="{}", algorithm=MD5)",
        DigestServerState::kRealm,
        DigestServerState::kNonce,
        DigestServerState::kQop
    );
    return response;
}

// Verifies the full end-to-end digest auth chain:
// - The generated client performs a Digest challenge/response
//   exchange on each request (libcurl receives a 401 with WWW-Authenticate,
//   then retries with a valid Authorization: Digest header).
// - The computed response digest matches the server-side expectation for the
//   given credentials, proving that username/password are wired correctly.
// - Multiple sequential calls each succeed, confirming the chain is repeatable.
UTEST(DigestAuth, CredentialsPassedAndServerValidatesDigest) {
    constexpr std::string_view kUsername = "testuser";
    constexpr std::string_view kPassword = "testpass";

    DigestServerState state;
    state.username = std::string{kUsername};
    state.password = std::string{kPassword};

    int successful_auth_count = 0;

    const utest::HttpServerMock http_server{[&](const utest::HttpServerMock::HttpRequest& request) {
        if (!state.ValidateAuth(request)) {
            // Issue a fresh challenge; libcurl will retry with credentials.
            return MakeDigestChallenge();
        }
        ++successful_auth_count;
        utest::HttpServerMock::HttpResponse response;
        response.response_status = 200;
        return response;
    }};

    chaotic::openapi::client::Config config;
    config.base_url = http_server.GetBaseUrl() + "/";
    config.digest_auth_credentials["myDigestScheme"] = chaotic::openapi::client::DigestAuthCredentials{
        std::string{kUsername},
        chaotic::openapi::client::DigestAuthCredentials::Password{std::string{kPassword}},
    };

    auto http_client_ptr = utest::CreateHttpClient();
    client_ns::ClientImpl client(config, *http_client_ptr);

    // First call: libcurl receives a 401, then re-sends with a valid digest.
    client.GetResource();
    EXPECT_TRUE(state.last_auth_valid);
    EXPECT_EQ(state.last_nc, 1u);
    EXPECT_EQ(successful_auth_count, 1);

    // Second call: the chain works again with the correct digest.
    state.last_auth_valid = false;
    client.GetResource();
    EXPECT_TRUE(state.last_auth_valid);
    EXPECT_EQ(state.last_nc, 1u);
    EXPECT_EQ(successful_auth_count, 2);
}

}  // namespace

USERVER_NAMESPACE_END
