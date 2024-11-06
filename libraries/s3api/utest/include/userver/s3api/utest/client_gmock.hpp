#pragma once

#include <userver/s3api/clients/s3api.hpp>

#include <gmock/gmock.h>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api {

class GMockClient : public Client {
public:
    MOCK_METHOD(
        std::string,
        PutObject,
        (std::string_view path,
         std::string data,
         const std::optional<Meta>& meta,
         std::string_view content_type,
         const std::optional<std::string>& content_disposition,
         const std::optional<std::vector<Tag>>& tags),
        (const, override)
    );

    MOCK_METHOD(void, DeleteObject, (std::string_view path), (const, override));

    MOCK_METHOD(
        std::optional<std::string>,
        GetObject,
        (std::string_view path,
         std::optional<std::string> version,
         HeadersDataResponse* headers_data,
         const HeaderDataRequest& headers_request),
        (const, override)
    );

    MOCK_METHOD(
        std::string,
        TryGetObject,
        (std::string_view path,
         std::optional<std::string> version,
         HeadersDataResponse* headers_data,
         const HeaderDataRequest& headers_request),
        (const, override)
    );

    MOCK_METHOD(
        std::optional<std::string>,
        GetPartialObject,
        (std::string_view path,
         std::string_view range,
         std::optional<std::string> version,
         HeadersDataResponse* headers_data,
         const HeaderDataRequest& headers_request),
        (const, override)
    );

    MOCK_METHOD(
        std::string,
        TryGetPartialObject,
        (std::string_view path,
         std::string_view range,
         std::optional<std::string> version,
         HeadersDataResponse* headers_data,
         const HeaderDataRequest& headers_request),
        (const, override)
    );

    MOCK_METHOD(
        std::string,
        CopyObject,
        (std::string_view key_from, std::string_view bucket_to, std::string_view key_to, const std::optional<Meta>& meta
        ),
        (override)
    );

    MOCK_METHOD(
        std::string,
        CopyObject,
        (std::string_view key_from, std::string_view key_to, const std::optional<Meta>& meta),
        (override)
    );

    MOCK_METHOD(
        std::optional<HeadersDataResponse>,
        GetObjectHead,
        (std::string_view path, const HeaderDataRequest& request),
        (const, override)
    );

    MOCK_METHOD(
        std::string,
        GenerateDownloadUrl,
        (std::string_view path, time_t expires, bool use_ssl),
        (const, override)
    );

    MOCK_METHOD(
        std::string,
        GenerateDownloadUrlVirtualHostAddressing,
        (std::string_view path, const std::chrono::system_clock::time_point& expires_at, std::string_view protocol),
        (const, override)
    );

    MOCK_METHOD(
        std::string,
        GenerateUploadUrlVirtualHostAddressing,
        (std::string_view data,
         std::string_view content_type,
         std::string_view path,
         const std::chrono::system_clock::time_point& expires_at,
         std::string_view protocol),
        (const, override)
    );

    MOCK_METHOD(
        std::optional<std::string>,
        ListBucketContents,
        (std::string_view path, int max_keys, std::string marker, std::string delimiter),
        (const, override)
    );

    MOCK_METHOD(std::vector<ObjectMeta>, ListBucketContentsParsed, (std::string_view path_prefix), (const, override));

    MOCK_METHOD(std::vector<std::string>, ListBucketDirectories, (std::string_view path_prefix), (const, override));

    MOCK_METHOD(void, UpdateConfig, (ConnectionCfg && config), (override));

    MOCK_METHOD(std::string_view, GetBucketName, (), (const, override));
};

}  // namespace s3api

USERVER_NAMESPACE_END
