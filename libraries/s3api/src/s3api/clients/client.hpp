#pragma once

/// @file userver/s3api/clients/s3api.hpp
/// @brief Client for any S3 api service
///

#include <chrono>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <userver/clients/http/request.hpp>
#include <userver/clients/http/response.hpp>
#include <userver/utils/overloaded.hpp>
#include <userver/utils/str_icase.hpp>

#include <userver/s3api/clients/s3api.hpp>

#include <userver/s3api/authenticators/access_key.hpp>
#include <userver/s3api/authenticators/interface.hpp>
#include <userver/s3api/models/fwd.hpp>
#include <userver/s3api/models/s3api_connection_type.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {
class Client;
}

namespace s3api {

/// Implementation of S3 client
class ClientImpl : public Client {
public:
    ClientImpl(
        std::shared_ptr<S3Connection> s3conn,
        std::shared_ptr<authenticators::Authenticator> authenticator,
        std::string bucket
    );
    // Deprecated, use constructor with general Authenticator interface
    ClientImpl(
        std::shared_ptr<S3Connection> s3conn,
        std::shared_ptr<authenticators::AccessKey> authenticator,
        std::string bucket
    );
    ~ClientImpl() override = default;

    std::string PutObject(
        std::string_view path,
        std::string data,
        const std::optional<Meta>& meta,
        std::string_view content_type,
        const std::optional<std::string>& content_disposition,
        const std::optional<std::vector<Tag>>& tags
    ) const final;

    void DeleteObject(std::string_view path) const final;

    std::optional<std::string> GetObject(
        std::string_view path,
        std::optional<std::string> versiong,
        HeadersDataResponse* headers_data,
        const HeaderDataRequest& headers_request
    ) const final;

    std::string TryGetObject(
        std::string_view path,
        std::optional<std::string> versiong,
        HeadersDataResponse* headers_data,
        const HeaderDataRequest& headers_request
    ) const final;

    std::optional<std::string> GetPartialObject(
        std::string_view path,
        std::string_view range,
        std::optional<std::string> versiong,
        HeadersDataResponse* headers_data,
        const HeaderDataRequest& headers_request
    ) const final;

    std::string TryGetPartialObject(
        std::string_view path,
        std::string_view range,
        std::optional<std::string> versiong,
        HeadersDataResponse* headers_data,
        const HeaderDataRequest& headers_request
    ) const final;

    std::string CopyObject(
        std::string_view key_from,
        std::string_view bucket_to,
        std::string_view key_to,
        const std::optional<Meta>& meta
    ) final;

    std::string CopyObject(std::string_view key_from, std::string_view key_to, const std::optional<Meta>& meta) final;

    std::optional<HeadersDataResponse> GetObjectHead(std::string_view path, const HeaderDataRequest& request)
        const final;

    std::string GenerateDownloadUrl(std::string_view path, time_t expires, bool use_ssl) const final;

    std::string GenerateDownloadUrlVirtualHostAddressing(
        std::string_view path,
        const std::chrono::system_clock::time_point& expires_at,
        std::string_view protocol
    ) const final;

    std::string GenerateUploadUrlVirtualHostAddressing(
        std::string_view data,
        std::string_view content_type,
        std::string_view path,
        const std::chrono::system_clock::time_point& expires_at,
        std::string_view protocol
    ) const final;

    std::optional<std::string>
    ListBucketContents(std::string_view path, int max_keys, std::string marker, std::string delimiter) const final;

    std::vector<ObjectMeta> ListBucketContentsParsed(std::string_view path_prefix) const final;

    std::vector<std::string> ListBucketDirectories(std::string_view path_prefix) const final;

    void UpdateConfig(ConnectionCfg&& config) final;

    std::string_view GetBucketName() const final;

private:
    void Auth(Request& request) const;
    std::string RequestApi(
        Request& request,
        std::string_view method_name,
        HeadersDataResponse* headers_data = nullptr,
        const HeaderDataRequest& headers_request = HeaderDataRequest()
    ) const;

    std::shared_ptr<S3Connection> conn_;
    std::shared_ptr<authenticators::Authenticator> authenticator_;
    std::string bucket_;
};

}  // namespace s3api

USERVER_NAMESPACE_END
