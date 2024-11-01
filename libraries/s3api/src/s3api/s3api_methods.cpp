#include "s3api_methods.hpp"

#include <fmt/format.h>

#include <unordered_map>

#include <userver/http/common_headers.hpp>
#include <userver/http/url.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api::api_methods {

Request PutObject(
    std::string_view bucket,
    std::string_view path,
    std::string data,
    std::string_view content_type,
    const std::optional<std::string_view>& content_disposition
) {
    Request req;
    req.method = clients::http::HttpMethod::kPut;
    req.bucket = bucket;
    req.req = path;

    req.headers[USERVER_NAMESPACE::http::headers::kContentLength] = std::to_string(data.size());
    req.headers[USERVER_NAMESPACE::http::headers::kContentType] = content_type;

    if (content_disposition.has_value()) {
        req.headers[USERVER_NAMESPACE::http::headers::kContentDisposition] = *content_disposition;
    }

    req.body = std::move(data);
    return req;
}

Request DeleteObject(std::string_view bucket, std::string_view path) {
    Request req;
    req.method = clients::http::HttpMethod::kDelete;
    req.bucket = bucket;
    req.req = path;
    return req;
}

Request GetObject(std::string_view bucket, std::string_view path, std::optional<std::string_view> version) {
    Request req;
    req.method = clients::http::HttpMethod::kGet;
    req.bucket = bucket;
    req.req = path;

    if (version) {
        req.req += "?" + USERVER_NAMESPACE::http::MakeQuery({{"versionId", *version}});
    }

    return req;
}

Request GetObjectHead(std::string_view bucket, std::string_view path) {
    Request req;
    req.method = clients::http::HttpMethod::kHead;
    req.bucket = bucket;
    req.req = path;
    return req;
}

void SetRange(Request& req, size_t begin, size_t end) {
    req.headers[USERVER_NAMESPACE::http::headers::kRange] =
        "bytes=" + std::to_string(begin) + '-' + std::to_string(end);
}

Request GetBuckets() { return Request{{}, "", "", "", clients::http::HttpMethod::kGet}; }

Request ListBucketContents(
    std::string_view bucket,
    std::string_view path,
    int max_keys,
    std::string_view marker,
    std::string_view delimiter
) {
    Request req;
    req.method = clients::http::HttpMethod::kGet;
    req.bucket = bucket;

    std::unordered_map<std::string, std::string> params;
    params.emplace("prefix", path);
    if (max_keys > 0) {
        params.emplace("max-keys", std::to_string(max_keys));
    }

    if (!marker.empty()) {
        params.emplace("marker", marker);
    }

    if (!delimiter.empty()) {
        params.emplace("delimiter", delimiter);
    }

    req.req = "?" + USERVER_NAMESPACE::http::MakeQuery(params);
    return req;
}
Request CopyObject(
    std::string_view source_bucket,
    std::string_view source_key,
    std::string_view dest_bucket,
    std::string_view dest_key,
    std::string_view content_type
) {
    Request req;
    req.method = clients::http::HttpMethod::kPut;
    req.bucket = dest_bucket;
    req.req = dest_key;

    req.headers[headers::kAmzCopySource] = fmt::format("/{}/{}", source_bucket, source_key);
    req.headers[USERVER_NAMESPACE::http::headers::kContentType] = content_type;

    return req;
}

}  // namespace s3api::api_methods

USERVER_NAMESPACE_END
