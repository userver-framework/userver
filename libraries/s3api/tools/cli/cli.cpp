#include "cli.hpp"

#include <chrono>
#include <memory>
#include <stdexcept>
#include <unordered_set>
#include <utility>

#include <userver/http/common_headers.hpp>
#include <userver/s3api/authenticators/access_key.hpp>
#include <userver/s3api/authenticators/signature_v4.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api::cli {

ClientPtr MakeClient(clients::http::Client& http_client, const Options& options, const Credentials& credentials) {
    const auto connection_type =
        options.endpoint.starts_with(kHttpsScheme) ? S3ConnectionType::kHttps : S3ConnectionType::kHttp;
    auto connection = MakeS3Connection(
        http_client,
        connection_type,
        options.endpoint,
        ConnectionCfg{std::chrono::milliseconds{options.timeout_ms}, options.retries}
    );
    std::shared_ptr<authenticators::Authenticator> authenticator;
    if (options.signature_version == "v2") {
        authenticator = std::make_shared<authenticators::AccessKey>(credentials.access_key, credentials.secret_key);
    } else {
        authenticator = std::make_shared<
            authenticators::SignatureV4>(credentials.access_key, credentials.secret_key, options.region.value());
    }
    return GetS3Client(std::move(connection), std::move(authenticator), options.bucket);
}

namespace {

Result GetObject(const Client& client, const Options& options) {
    Client::HeadersDataResponse response;
    auto contents = client.TryGetObject(
        options.key,
        std::nullopt,
        &response,
        Client::HeaderDataRequest{std::unordered_set<std::string>{"ETag"}, false}
    );
    if (!response.headers) {
        throw std::runtime_error("S3 GET response has no headers");
    }
    const auto etag = response.headers->find(http::headers::kETag);
    if (etag == response.headers->end() || etag->second.empty()) {
        throw std::runtime_error("S3 GET response has no nonempty ETag");
    }
    return {std::move(contents), etag->second};
}

Result PutObject(const Client& client, const Options& options, std::string body) {
    client.PutObject(options.key, std::move(body));
    return {};
}

Result PutObjectConditional(const Client& client, const Options& options, std::string body) {
    UASSERT(options.if_match.has_value() != options.if_none_match);
    client.PutObjectConditional(options.key, std::move(body), options.if_match);
    return {};
}

}  // namespace

Result Execute(const Client& client, const Options& options, std::string body) {
    switch (options.command) {
        case Command::kGetObject:
            return GetObject(client, options);
        case Command::kPutObject:
            return PutObject(client, options, std::move(body));
        case Command::kPutObjectConditional:
            return PutObjectConditional(client, options, std::move(body));
    }
    throw std::invalid_argument("Unsupported S3 CLI command");
}

}  // namespace s3api::cli

USERVER_NAMESPACE_END
