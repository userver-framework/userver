#pragma once

/// @file userver/s3api/clients/s3api.hpp
/// @brief Client for any S3 api service

#include <chrono>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include <userver/http/header_map.hpp>

#include <userver/s3api/authenticators/access_key.hpp>
#include <userver/s3api/authenticators/interface.hpp>
#include <userver/s3api/models/fwd.hpp>
#include <userver/s3api/models/s3api_connection_type.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {
class Client;
}

/// @brief Top namespace for S3 library.
///
/// For more information see @ref s3_library.
namespace s3api {

class AuthHeaderConflictError : public std::runtime_error {
    using runtime_error::runtime_error;
};

class NoBucketError : public std::runtime_error {
    using runtime_error::runtime_error;
};

class ListBucketError : public std::runtime_error {
    using runtime_error::runtime_error;
};

/// Connection settings - retries, timeouts, and so on
struct ConnectionCfg {
    explicit ConnectionCfg(
        std::chrono::milliseconds s3timeout,
        int s3retries = 1,
        std::optional<std::string> proxy = {}
    )
        : timeout(s3timeout), retries(s3retries), proxy(proxy) {}

    std::chrono::milliseconds timeout{1000};
    int retries = 1;
    std::optional<std::string> proxy{};
};

/// https://docs.aws.amazon.com/AmazonS3/latest/API/API_ListObjects.html
/// may also include other fields like Owner, ETag, etc.
struct ObjectMeta {
    std::string key;
    std::size_t size;
    std::string last_modified;
};

/// Represents a connection to s3 api. This object is only forward-declared,
/// with private implementation (mostly because it is very very ugly)
class S3Connection;

/// Create an S3Connection object. By itself, it does nothing, but you need
/// one to create S3 client
std::shared_ptr<S3Connection> MakeS3Connection(
    clients::http::Client& http_client,
    S3ConnectionType connection_type,
    std::string server_url,
    const ConnectionCfg& params
);

/// Main interface for the S3 api
class Client {
public:
    using CiUnorderedMap = USERVER_NAMESPACE::http::headers::HeaderMap;
    using Meta = CiUnorderedMap;
    using Headers = CiUnorderedMap;

    struct HeaderDataRequest {
        HeaderDataRequest() {}
        HeaderDataRequest(std::optional<std::unordered_set<std::string>> headers, bool need_meta)
            : headers(std::move(headers)), need_meta(need_meta) {}
        std::optional<std::unordered_set<std::string>> headers{std::nullopt};
        bool need_meta{true};
    };

    struct HeadersDataResponse {
        std::optional<Headers> headers;
        std::optional<Meta> meta;
    };

    struct Tag {
        std::string key;
        std::string value;
    };

    virtual ~Client() = default;

    // NOLINTBEGIN(google-default-arguments)

    virtual std::string PutObject(
        std::string_view path,
        std::string data,
        const std::optional<Meta>& meta = std::nullopt,
        std::string_view content_type = "text/plain",
        const std::optional<std::string>& content_disposition = std::nullopt,
        const std::optional<std::vector<Tag>>& tags = std::nullopt
    ) const = 0;

    virtual void DeleteObject(std::string_view path) const = 0;

    virtual std::optional<std::string> GetObject(
        std::string_view path,
        std::optional<std::string> version = std::nullopt,
        HeadersDataResponse* headers_data = nullptr,
        const HeaderDataRequest& headers_request = HeaderDataRequest()
    ) const = 0;

    virtual std::string TryGetObject(
        std::string_view path,
        std::optional<std::string> version = std::nullopt,
        HeadersDataResponse* headers_data = nullptr,
        const HeaderDataRequest& headers_request = HeaderDataRequest()
    ) const = 0;

    virtual std::string CopyObject(
        std::string_view key_from,
        std::string_view bucket_to,
        std::string_view key_to,
        const std::optional<Meta>& meta = std::nullopt
    ) = 0;

    virtual std::string
    CopyObject(std::string_view key_from, std::string_view key_to, const std::optional<Meta>& meta = std::nullopt) = 0;

    virtual std::optional<HeadersDataResponse>
    GetObjectHead(std::string_view path, const HeaderDataRequest& request = HeaderDataRequest()) const = 0;

    virtual std::string GenerateDownloadUrl(std::string_view path, time_t expires, bool use_ssl = false) const = 0;

    virtual std::string GenerateDownloadUrlVirtualHostAddressing(
        std::string_view path,
        const std::chrono::system_clock::time_point& expires_at,
        std::string_view protocol = "https://"
    ) const = 0;

    virtual std::string GenerateUploadUrlVirtualHostAddressing(
        std::string_view data,
        std::string_view content_type,
        std::string_view path,
        const std::chrono::system_clock::time_point& expires_at,
        std::string_view protocol = "https://"
    ) const = 0;

    virtual std::optional<std::string> ListBucketContents(
        std::string_view path,
        int max_keys,
        std::string marker = "",
        std::string delimiter = ""
    ) const = 0;

    virtual std::vector<ObjectMeta> ListBucketContentsParsed(std::string_view path_prefix) const = 0;

    virtual std::vector<std::string> ListBucketDirectories(std::string_view path_prefix) const = 0;

    virtual void UpdateConfig(ConnectionCfg&& config) = 0;

    virtual std::string_view GetBucketName() const = 0;

    // NOLINTEND(google-default-arguments)
};

using ClientPtr = std::shared_ptr<Client>;

ClientPtr GetS3Client(
    std::shared_ptr<S3Connection> s3conn,
    std::shared_ptr<authenticators::AccessKey> authenticator,
    std::string bucket
);

ClientPtr GetS3Client(
    std::shared_ptr<S3Connection> s3conn,
    std::shared_ptr<authenticators::Authenticator> authenticator,
    std::string bucket
);

}  // namespace s3api

USERVER_NAMESPACE_END
