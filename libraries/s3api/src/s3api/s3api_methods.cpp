#include "s3api_methods.hpp"

#include <fmt/format.h>
#include <pugixml.hpp>

#include <unordered_map>

#include <userver/http/common_headers.hpp>
#include <userver/http/url.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api::api_methods {
namespace {

class StringWriter : public pugi::xml_writer {
public:
    explicit StringWriter(std::string& out)
        : out_(out)
    {}

    void write(const void* data, size_t size) override { out_.append(static_cast<const char*>(data), size); }

private:
    std::string& out_;
};

namespace query_args {
constexpr std::string_view kDelimeter = "delimiter";
constexpr std::string_view kEncodingType = "encoding-type";
constexpr std::string_view kMarker = "marker";
constexpr std::string_view kMaxKeys = "max-keys";
constexpr std::string_view kMaxParts = "max-parts";
constexpr std::string_view kMaxUploads = "max-uploads";
constexpr std::string_view kPartNumber = "partNumber";
constexpr std::string_view kPartNumberMarker = "part-number-marker";
constexpr std::string_view kPrefix = "prefix";
constexpr std::string_view kUploadId = "uploadId";
constexpr std::string_view kUploadIdMarker = "upload-id-marker";
}  // namespace query_args

namespace content_types {
constexpr std::string_view kDefault = "application/octet-stream";
constexpr std::string_view kApplicationXml = "application/xml; charset=UTF-8";
}  // namespace content_types

constexpr std::string_view kEncodingTypeValueUrl = "url";

constexpr std::string_view kS3AwsXmlNamespace = "http://s3.amazonaws.com/doc/2006-03-01/";

// see https://docs.aws.amazon.com/AmazonS3/latest/API/API_ListMultipartUploads.html
const unsigned kMaxUploadsListLimit = 1000u;

}  // namespace

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
    req.headers
        [USERVER_NAMESPACE::http::headers::kRange] = "bytes=" + std::to_string(begin) + '-' + std::to_string(end);
}

void SetRange(Request& req, std::string_view range) { req.headers[USERVER_NAMESPACE::http::headers::kRange] = range; }

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
        params.emplace(query_args::kMaxKeys, std::to_string(max_keys));
    }

    if (!marker.empty()) {
        params.emplace(query_args::kMarker, marker);
    }

    if (!delimiter.empty()) {
        params.emplace(query_args::kDelimeter, delimiter);
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

Request CreateInternalApiRequest(
    const std::string& bucket,
    const multipart_upload::CreateMultipartUploadRequest& request
) {
    Request result;
    result.method = clients::http::HttpMethod::kPost;
    result.bucket = bucket;
    result.req = fmt::format("{}?uploads", request.key);

    if (request.content_type) {
        result.headers[http::headers::kContentType] = *request.content_type;
    }

    if (request.content_disposition) {
        result.headers[http::headers::kContentDisposition] = *request.content_disposition;
    }

    if (request.content_encoding) {
        result.headers[http::headers::kContentEncoding] = *request.content_encoding;
    }

    if (request.content_language) {
        result.headers[http::headers::kContentLanguage] = *request.content_language;
    }

    return result;
}

Request CreateInternalApiRequest(
    const std::string& bucket,
    const multipart_upload::AbortMultipartUploadRequest& request
) {
    Request result;
    result.method = clients::http::HttpMethod::kDelete;
    result.bucket = bucket;
    result.req = fmt::format("{}?{}", request.key, http::MakeQuery({{query_args::kUploadId, request.upload_id}}));
    return result;
}

Request CreateInternalApiRequest(
    const std::string& bucket,
    const multipart_upload::CompleteMultipartUploadRequest& request
) {
    Request result;
    result.method = clients::http::HttpMethod::kPost;
    result.bucket = bucket;
    result.req = fmt::format("{}?{}", request.key, http::MakeQuery({{query_args::kUploadId, request.upload_id}}));

    pugi::xml_document doc;
    auto multipart_upload_node = doc.append_child("CompleteMultipartUpload");
    multipart_upload_node.append_attribute("xmlns").set_value(kS3AwsXmlNamespace.data());
    for (const auto& part : request.completed_parts) {
        auto part_node = multipart_upload_node.append_child("Part");
        part_node.append_child("PartNumber")
            .append_child(pugi::node_pcdata)
            .set_value(std::to_string(part.part_number).c_str());
        part_node.append_child("ETag").append_child(pugi::node_pcdata).set_value(part.etag.c_str());
    }
    StringWriter writer{result.body};
    doc.save(writer, "", pugi::format_raw);
    result.headers = clients::http::Headers{
        {http::headers::kContentType, content_types::kApplicationXml},
        {http::headers::kContentLength, std::to_string(result.body.size())}
    };

    return result;
}

Request CreateInternalApiRequest(const std::string& bucket, const multipart_upload::UploadPartRequest& request) {
    Request result;
    result.method = clients::http::HttpMethod::kPut;
    // `Content-Type`-header is absent in S3 API, however PUT-request implementaion in CURL for URIs with query
    // may implicitly insert `application/x-www-form-urlencoded` which must be taken into account when making
    // canonical request for signature. But it happens after we sign the request without knowlege of this header.
    // We explicitly override this Content-Type value here to make this request be signed correctly.
    result.headers = clients::http::Headers{
        {http::headers::kContentLength, std::to_string(request.data.size())},
        {http::headers::kContentType, std::string{content_types::kDefault}}
    };

    result.bucket = bucket;
    result.req = fmt::format(
        "{}?{}",
        request.key,
        http::MakeQuery(
            {{query_args::kUploadId, request.upload_id}, {query_args::kPartNumber, std::to_string(request.part_number)}}
        )
    );
    result.body = std::move(request.data);
    return result;
}

Request CreateInternalApiRequest(const std::string& bucket, const multipart_upload::ListPartsRequest& request) {
    Request result;
    result.method = clients::http::HttpMethod::kGet;
    result.bucket = bucket;
    result.req = request.key;

    http::Args params{{std::string{query_args::kUploadId}, request.upload_id}};

    if (request.max_parts > 0 && request.max_parts < kMaxUploadsListLimit) {
        params.emplace(query_args::kMaxParts, std::to_string(request.max_parts));
    }

    if (auto part_number_marker = request.part_number_marker.value_or(0u)) {
        params.emplace(query_args::kPartNumberMarker, std::to_string(part_number_marker));
    }

    result.req = fmt::format("{}?{}", request.key, http::MakeQuery(params));

    return result;
}

Request CreateInternalApiRequest(
    const std::string& bucket,
    const multipart_upload::ListMultipartUploadsRequest& request
) {
    using EncodingType = multipart_upload::ListMultipartUploadsRequest::EncodingType;

    Request result;
    result.method = clients::http::HttpMethod::kGet;
    result.bucket = bucket;

    http::Args params;
    if (request.delimiter) {
        params.emplace(query_args::kDelimeter, *request.delimiter);
    }

    if (request.prefix) {
        params.emplace(query_args::kPrefix, *request.prefix);
    }

    if (request.max_uploads > 0 && request.max_uploads < kMaxUploadsListLimit) {
        params.emplace(query_args::kMaxUploads, std::to_string(request.max_uploads));
    }

    if (request.key_marker) {
        params.emplace(query_args::kMarker, *request.key_marker);
    }

    if (request.upload_id_marker) {
        params.emplace(query_args::kUploadIdMarker, *request.upload_id_marker);
    }

    if (request.encoding_type == EncodingType::kUrl) {
        params.emplace(query_args::kEncodingType, kEncodingTypeValueUrl);
    }

    result.req = "?uploads";
    if (!params.empty()) {
        result.req += "&" + http::MakeQuery(params);
    }

    return result;
}

}  // namespace s3api::api_methods

USERVER_NAMESPACE_END
