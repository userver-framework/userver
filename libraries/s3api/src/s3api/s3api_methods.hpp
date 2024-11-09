#pragma once

#include <optional>
#include <string>

#include <userver/http/predefined_header.hpp>

#include <userver/s3api/models/request.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api::api_methods {

namespace headers {
inline constexpr USERVER_NAMESPACE::http::headers::PredefinedHeader kAmzCopySource{"x-amz-copy-source"};
}

Request PutObject(
    std::string_view bucket,
    std::string_view path,
    std::string data,
    std::string_view content_type,
    const std::optional<std::string_view>& content_disposition = std::nullopt
);

Request DeleteObject(std::string_view bucket, std::string_view path);

Request
GetObject(std::string_view bucket, std::string_view path, std::optional<std::string_view> version = std::nullopt);

Request GetObjectHead(std::string_view bucket, std::string_view path);

void SetRange(Request& req, size_t begin, size_t end);

void SetRange(Request& req, std::string_view range);

Request GetBuckets();

Request ListBucketContents(
    std::string_view bucket,
    std::string_view path,
    int max_keys = 0,
    std::string_view marker = "",
    std::string_view delimiter = ""
);

Request CopyObject(
    std::string_view source_bucket,
    std::string_view source_key,
    std::string_view dest_bucket,
    std::string_view dest_key,
    std::string_view content_type
);

}  // namespace s3api::api_methods

USERVER_NAMESPACE_END
