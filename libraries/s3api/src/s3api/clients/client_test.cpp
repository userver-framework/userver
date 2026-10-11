#include <s3api/clients/client.hpp>

#include <array>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <userver/clients/http/client.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/s3api/models/request.hpp>
#include <userver/utest/utest.hpp>

#include <s3api/s3_connection.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api {
namespace {

class UnusedHttpClient final : public clients::http::Client {
public:
    clients::http::Request CreateRequest() override { throw std::logic_error("HTTP request is not expected"); }
};

class HostCapturingAuthenticator final : public authenticators::Authenticator {
public:
    std::unordered_map<std::string, std::string> Auth(const Request&) const override { return {}; }

    std::unordered_map<std::string, std::string> Sign(const Request& request, std::time_t) const override {
        const auto it = request.headers.find(http::headers::kHost);
        if (it == request.headers.end()) {
            throw std::logic_error("Host header is missing");
        }
        signed_host = it->second;
        return {};
    }

    mutable std::string signed_host;
};

class ConditionalHeadersAuthenticator final : public authenticators::Authenticator {
public:
    std::unordered_map<std::string, std::string> Auth(const Request& request) const override {
        const auto match = request.headers.find(http::headers::kIfMatch);
        const auto none_match = request.headers.find(http::headers::kIfNoneMatch);
        if_match = match == request.headers.end() ? "" : match->second;
        if_none_match = none_match == request.headers.end() ? "" : none_match->second;
        throw std::runtime_error("Authentication captured the conditional request");
    }

    std::unordered_map<std::string, std::string> Sign(const Request&, std::time_t) const override {
        throw std::logic_error("Presigning is not expected");
    }

    mutable std::string if_match;
    mutable std::string if_none_match;
};

UTEST(S3ConditionalWrite, SignsCreationAndReplacementPreconditions) {
    UnusedHttpClient http_client;
    auto connection = std::make_shared<S3Connection>(
        http_client,
        S3ConnectionType::kHttps,
        "s3.example.com",
        ConnectionCfg{std::chrono::milliseconds{1000}}
    );
    auto authenticator = std::make_shared<ConditionalHeadersAuthenticator>();
    ClientImpl client{std::move(connection), authenticator, "bucket"};

    EXPECT_THROW(client.PutObjectConditional("head", "data", std::nullopt), std::runtime_error);
    EXPECT_EQ(authenticator->if_none_match, "*");
    EXPECT_TRUE(authenticator->if_match.empty());

    EXPECT_THROW(client.PutObjectConditional("head", "data", "\"previous\""), std::runtime_error);
    EXPECT_EQ(authenticator->if_match, "\"previous\"");
    EXPECT_TRUE(authenticator->if_none_match.empty());

    EXPECT_THROW(client.PutObjectConditional("head", "data", ""), std::invalid_argument);
}

TEST(S3PresignedUrl, NormalizesEndpointHostForDownloadAndUpload) {
    UnusedHttpClient http_client;
    const auto expires_at = std::chrono::system_clock::time_point{std::chrono::seconds{1'700'000'000}};

    struct EndpointCase {
        const char* endpoint;
        const char* expected_host;
        const char* expected_endpoint_path;
    };
    // Secdist endpoints may include a scheme and path; sign only Host and keep the path in the URL.
    constexpr std::array endpoints{
        EndpointCase{"s3.example.com", "bucket.s3.example.com", ""},
        EndpointCase{"http://s3.example.com", "bucket.s3.example.com", ""},
        EndpointCase{"https://s3.example.com", "bucket.s3.example.com", ""},
        EndpointCase{"s3.example.com/mds-s3", "bucket.s3.example.com", "/mds-s3"},
        EndpointCase{"http://localhost:41871/s3mds", "bucket.localhost:41871", "/s3mds"},
        EndpointCase{"https://localhost:41871/mds-s3-staff/", "bucket.localhost:41871", "/mds-s3-staff"},
    };

    for (const auto& [endpoint, expected_host, expected_endpoint_path] : endpoints) {
        auto connection = std::make_shared<S3Connection>(
            http_client,
            S3ConnectionType::kHttps,
            endpoint,
            ConnectionCfg{std::chrono::milliseconds{1000}}
        );
        auto authenticator = std::make_shared<HostCapturingAuthenticator>();
        ClientImpl client{std::move(connection), authenticator, "bucket"};

        for (const auto* protocol : std::array{"http://", "https://"}) {
            const auto
                expected_url = std::string{protocol} + expected_host + expected_endpoint_path + "/path/to/object";
            EXPECT_EQ(
                client.GenerateDownloadUrlVirtualHostAddressing("path/to/object", expires_at, protocol),
                expected_url
            );
            EXPECT_EQ(authenticator->signed_host, expected_host);

            EXPECT_EQ(
                client.GenerateUploadUrlVirtualHostAddressing(
                    "content",
                    "text/plain",
                    "path/to/object",
                    expires_at,
                    protocol
                ),
                expected_url
            );
            EXPECT_EQ(authenticator->signed_host, expected_host);
        }
    }
}

}  // namespace
}  // namespace s3api

USERVER_NAMESPACE_END
