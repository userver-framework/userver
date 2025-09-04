#include <s3api/clients/client.hpp>

#include <sstream>

#include <fmt/format.h>
#include <boost/algorithm/string.hpp>
#include <pugixml.hpp>

#include <userver/http/common_headers.hpp>
#include <userver/http/url.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/exception.hpp>

#include <userver/s3api/authenticators/access_key.hpp>

#include <s3api/s3_connection.hpp>
#include <s3api/s3api_methods.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api {
namespace {

const std::string kMeta = "x-amz-meta-";
const std::string kTagging = "X-Amz-Tagging";
constexpr const std::size_t kMaxS3Keys = 1000;

void SaveMeta(clients::http::Headers& headers, const ClientImpl::Meta& meta) {
    for (const auto& [header, value] : meta) {
        headers[kMeta + header] = value;
    }
}

void ReadMeta(const clients::http::Headers& headers, ClientImpl::Meta& meta) {
    for (const auto& [header, value] : headers) {
        if (boost::istarts_with(header, kMeta)) {
            meta[header.substr(kMeta.length())] = value;
        }
    }
}

void SaveTags(clients::http::Headers& headers, const std::vector<ClientImpl::Tag>& tags) {
    size_t size = 0;
    for (const auto& [key, value] : tags) {
        size += key.size() + value.size() + 2;
    }
    std::string tag_values;
    tag_values.reserve(size);
    for (const auto& [key, value] : tags) {
        tag_values.append(key);
        tag_values.append("=");
        tag_values.append(value);
        tag_values.append("&");
    }
    if (!tag_values.empty()) {
        // pop last &
        tag_values.pop_back();
    }
    headers[kTagging] = std::move(tag_values);
}

void AddQueryParamsToPresignedUrl(
    std::ostringstream& generated_url,
    time_t expires_at,
    const Request& req,
    std::shared_ptr<authenticators::Authenticator> authenticator
) {
    if (!req.req.empty()) {
        generated_url << "/" + req.req;
    }

    auto params = authenticator->Sign(req, expires_at);

    if (!params.empty()) {
        generated_url << "?";
        generated_url << USERVER_NAMESPACE::http::MakeQuery(params);
    }
}

std::string GeneratePresignedUrl(
    const Request& request,
    std::string_view host,
    std::string_view protocol,
    const std::chrono::system_clock::time_point& expires_at,
    std::shared_ptr<authenticators::Authenticator> authenticator
) {
    std::ostringstream generated_url;
    // both internal (s3.mds(t)) and private (s3-private)
    // balancers support virtual host addressing and https
    generated_url << protocol << request.bucket << "." << host;
    const auto expires_at_time_t = std::chrono::system_clock::to_time_t(expires_at);
    AddQueryParamsToPresignedUrl(generated_url, expires_at_time_t, request, std::move(authenticator));
    return generated_url.str();
}

std::vector<ObjectMeta> ParseS3ListResponse(std::string_view s3_response) {
    std::vector<ObjectMeta> result;
    pugi::xml_document xml;
    const pugi::xml_parse_result parse_result = xml.load_string(s3_response.data());
    if (parse_result.status != pugi::status_ok) {
        throw ListBucketError(fmt::format(
            "Failed to parse S3 list response as xml, error: {}, response: {}", parse_result.description(), s3_response
        ));
    }
    try {
        const auto items = xml.child("ListBucketResult").children("Contents");
        for (const auto& item : items) {
            const auto key = item.child("Key").child_value();
            const auto size = std::stoull(item.child("Size").child_value());
            const auto last_modified = item.child("LastModified").child_value();
            result.push_back(ObjectMeta{key, size, last_modified});
        }
    } catch (const pugi::xpath_exception& ex) {
        throw ListBucketError(
            fmt::format("Bad xml structure for S3 list response, error: {}, response: {}", ex.what(), s3_response)
        );
    }
    return result;
}

std::vector<std::string> ParseS3DirectoriesListResponse(std::string_view s3_response) {
    std::vector<std::string> result;
    pugi::xml_document xml;
    const pugi::xml_parse_result parse_result = xml.load_string(s3_response.data());
    if (parse_result.status != pugi::status_ok) {
        throw ListBucketError(fmt::format(
            "Failed to parse S3 directories list response as xml, error: {}, "
            "response: {}",
            parse_result.description(),
            s3_response
        ));
    }
    try {
        const auto items = xml.child("ListBucketResult").children("CommonPrefixes");
        for (const auto& item : items) {
            result.push_back(item.child("Prefix").child_value());
        }
    } catch (const pugi::xpath_exception& ex) {
        throw ListBucketError(fmt::format(
            "Bad xml structure for S3 directories list response, error: {}, "
            "response: {}",
            ex.what(),
            s3_response
        ));
    }
    return result;
}

}  // namespace

void ClientImpl::UpdateConfig(ConnectionCfg&& config) { conn_->UpdateConfig(std::move(config)); }

ClientImpl::ClientImpl(
    std::shared_ptr<S3Connection> s3conn,
    std::shared_ptr<authenticators::Authenticator> authenticator,
    std::string bucket
)
    : conn_(std::move(s3conn)), authenticator_{std::move(authenticator)}, bucket_(std::move(bucket)) {}

ClientImpl::ClientImpl(
    std::shared_ptr<S3Connection> s3conn,
    std::shared_ptr<authenticators::AccessKey> authenticator,
    std::string bucket
)
    : ClientImpl(
          std::move(s3conn),
          std::static_pointer_cast<authenticators::Authenticator>(std::move(authenticator)),
          std::move(bucket)
      ) {}

std::string_view ClientImpl::GetBucketName() const { return bucket_; }

std::string ClientImpl::PutObject(
    std::string_view path,            //
    std::string data,                 //
    const std::optional<Meta>& meta,  //
    std::string_view content_type,    //
    const std::optional<std::string>& content_disposition,
    const std::optional<std::vector<Tag>>& tags
) const {
    auto req = api_methods::PutObject(  //
        bucket_,                        //
        path,                           //
        std::move(data),                //
        content_type,                   //
        content_disposition             //
    );

    if (meta.has_value()) {
        SaveMeta(req.headers, meta.value());
    }
    if (tags.has_value()) {
        SaveTags(req.headers, tags.value());
    }
    return RequestApi(req, "put_object");
}

void ClientImpl::DeleteObject(std::string_view path) const {
    auto req = api_methods::DeleteObject(bucket_, path);
    RequestApi(req, "delete_object");
}

std::optional<std::string> ClientImpl::GetObject(
    std::string_view path,
    std::optional<std::string> version,
    HeadersDataResponse* headers_data,
    const HeaderDataRequest& headers_request
) const {
    try {
        return std::make_optional(TryGetObject(path, std::move(version), headers_data, headers_request));
    } catch (const clients::http::HttpException& e) {
        if (e.code() == 404) {
            LOG_INFO() << "Can't get object with path: " << path << ", object not found:" << e.what();
        } else {
            LOG_ERROR() << "Can't get object with path: " << path << ", unknown error:" << e.what();
        }
        return std::nullopt;
    } catch (const std::exception& e) {
        LOG_ERROR() << "Can't get object with path: " << path << ", unknown error:" << e.what();
        return std::nullopt;
    }
}

std::string ClientImpl::TryGetObject(
    std::string_view path,
    std::optional<std::string> version,
    HeadersDataResponse* headers_data,
    const HeaderDataRequest& headers_request
) const {
    auto req = api_methods::GetObject(bucket_, path, std::move(version));
    return RequestApi(req, "get_object", headers_data, headers_request);
}

std::optional<std::string> ClientImpl::GetPartialObject(
    std::string_view path,
    std::string_view range,
    std::optional<std::string> version,
    HeadersDataResponse* headers_data,
    const HeaderDataRequest& headers_request
) const {
    try {
        return std::make_optional(TryGetPartialObject(path, range, std::move(version), headers_data, headers_request));
    } catch (const clients::http::HttpException& e) {
        if (e.code() == 404) {
            LOG_INFO() << "Can't get object with path: " << path << ", object not found:" << e.what();
        } else {
            LOG_ERROR() << "Can't get object with path: " << path << ", unknown error:" << e.what();
        }
        return std::nullopt;
    } catch (const std::exception& e) {
        LOG_ERROR() << "Can't get object with path: " << path << ", unknown error:" << e.what();
        return std::nullopt;
    }
}

std::string ClientImpl::TryGetPartialObject(
    std::string_view path,
    std::string_view range,
    std::optional<std::string> version,
    HeadersDataResponse* headers_data,
    const HeaderDataRequest& headers_request
) const {
    auto req = api_methods::GetObject(bucket_, path, std::move(version));
    api_methods::SetRange(req, range);
    return RequestApi(req, "get_object", headers_data, headers_request);
}

std::optional<ClientImpl::HeadersDataResponse>
ClientImpl::GetObjectHead(std::string_view path, const HeaderDataRequest& headers_request) const {
    HeadersDataResponse headers_data;
    auto req = api_methods::GetObjectHead(bucket_, path);
    try {
        RequestApi(req, "get_object_head", &headers_data, headers_request);
    } catch (const std::exception& e) {
        LOG_INFO() << "Can't get object with path: " << path << ", error:" << e.what();
        return std::nullopt;
    }
    return std::make_optional(std::move(headers_data));
}

[[deprecated]] std::string ClientImpl::GenerateDownloadUrl(std::string_view path, time_t expires_at, bool use_ssl)
    const {
    auto req = api_methods::GetObject(bucket_, path);

    std::ostringstream generated_url;
    auto host = conn_->GetHost();
    if (host.find("://") == std::string::npos) {
        generated_url << (use_ssl ? "https" : "http") << "://";
    }
    generated_url << host;

    if (!req.bucket.empty()) {
        generated_url << "/" + req.bucket;
    }
    AddQueryParamsToPresignedUrl(generated_url, expires_at, req, authenticator_);
    return generated_url.str();
}

std::string ClientImpl::GenerateDownloadUrlVirtualHostAddressing(
    std::string_view path,
    const std::chrono::system_clock::time_point& expires_at,
    std::string_view protocol
) const {
    auto req = api_methods::GetObject(bucket_, path);
    if (req.bucket.empty()) {
        throw NoBucketError("presigned url for empty bucket string");
    }
    return GeneratePresignedUrl(req, conn_->GetHost(), protocol, expires_at, authenticator_);
}

std::string ClientImpl::GenerateUploadUrlVirtualHostAddressing(
    std::string_view data,
    std::string_view content_type,
    std::string_view path,
    const std::chrono::system_clock::time_point& expires_at,
    std::string_view protocol
) const {
    auto req = api_methods::PutObject(bucket_, path, std::string{data}, content_type);
    if (req.bucket.empty()) {
        throw NoBucketError("presigned url for empty bucket string");
    }
    return GeneratePresignedUrl(req, conn_->GetHost(), protocol, expires_at, authenticator_);
}

void ClientImpl::Auth(Request& request) const {
    if (!authenticator_) {
        // anonymous request
        return;
    }

    auto auth_headers = authenticator_->Auth(request);

    {
        auto it = std::find_if(
            auth_headers.cbegin(),
            auth_headers.cend(),
            [&request](const decltype(auth_headers)::value_type& header) {
                return request.headers.count(std::get<0>(header));
            }
        );

        if (it != auth_headers.cend()) {
            throw AuthHeaderConflictError{std::string{"Conflict with auth header: "} + it->first};
        }
    }

    request.headers.insert(
        std::make_move_iterator(std::begin(auth_headers)), std::make_move_iterator(std::end(auth_headers))
    );
}

std::string ClientImpl::RequestApi(
    Request& request,
    std::string_view method_name,
    HeadersDataResponse* headers_data,
    const HeaderDataRequest& headers_request
) const {
    Auth(request);

    auto response = conn_->RequestApi(request, method_name);

    if (headers_data) {
        if (headers_request.need_meta) {
            headers_data->meta.emplace();
            ReadMeta(response->headers(), *headers_data->meta);
        }

        if (headers_request.headers) {
            headers_data->headers.emplace();
            for (const auto& header : *headers_request.headers) {
                if (auto it = response->headers().find(header); it != response->headers().end()) {
                    headers_data->headers->emplace(it->first, it->second);
                }
            }
        }
    }

    return response->body();
}

std::optional<std::string>
ClientImpl::ListBucketContents(std::string_view path, int max_keys, std::string marker, std::string delimiter) const {
    auto req = api_methods::ListBucketContents(bucket_, path, max_keys, marker, delimiter);
    std::string reply = RequestApi(req, "list_bucket_contents");
    if (reply.empty()) {
        return std::nullopt;
    }
    return std::optional<std::string>{std::move(reply)};
}

std::vector<ObjectMeta> ClientImpl::ListBucketContentsParsed(std::string_view path_prefix) const {
    std::vector<ObjectMeta> result;
    // S3 doc: specifies the key to start with when listing objects in a bucket
    std::string marker = "";
    bool is_finished = false;
    while (!is_finished) {
        auto response = ListBucketContents(path_prefix, kMaxS3Keys, marker, {});
        if (!response) {
            LOG_WARNING() << "Empty S3 bucket listing response for path prefix " << path_prefix;
            break;
        }
        auto response_result = ParseS3ListResponse(*response);
        if (response_result.empty()) {
            break;
        }
        if (response_result.size() < kMaxS3Keys) {
            is_finished = true;
        }
        result.insert(
            result.end(),
            std::make_move_iterator(response_result.begin()),
            std::make_move_iterator(response_result.end())
        );
        marker = result.back().key;
    }
    return result;
}

std::vector<std::string> ClientImpl::ListBucketDirectories(std::string_view path_prefix) const {
    std::vector<std::string> result;
    // S3 doc: specifies the key to start with when listing objects in a bucket
    std::string marker = "";
    bool is_finished = false;
    while (!is_finished) {
        auto response = ListBucketContents(path_prefix, kMaxS3Keys, marker, "/");
        if (!response) {
            LOG_WARNING() << "Empty S3 directory bucket listing response "
                             "for path prefix "
                          << path_prefix;
            break;
        }
        auto response_result = ParseS3DirectoriesListResponse(*response);
        if (response_result.empty()) {
            break;
        }
        if (response_result.size() < kMaxS3Keys) {
            is_finished = true;
        }
        result.insert(
            result.end(),
            std::make_move_iterator(response_result.begin()),
            std::make_move_iterator(response_result.end())
        );
        marker = result.back();
    }

    return result;
}

std::string ClientImpl::CopyObject(
    std::string_view key_from,
    std::string_view bucket_to,
    std::string_view key_to,
    const std::optional<Meta>& meta
) {
    const auto object_head = [&] {
        HeaderDataRequest header_request;
        header_request.headers.emplace();
        header_request.headers->emplace(USERVER_NAMESPACE::http::headers::kContentType);
        header_request.need_meta = false;
        return GetObjectHead(key_from, header_request);
    }();
    if (!object_head) {
        USERVER_NAMESPACE::utils::LogErrorAndThrow("S3Api : Failed to get object head");
    }

    const auto content_type = [&object_head]() -> std::optional<std::string> {
        if (!object_head->headers) {
            return std::nullopt;
        }

        return USERVER_NAMESPACE::utils::FindOptional(
            *object_head->headers, USERVER_NAMESPACE::http::headers::kContentType
        );
    }();
    if (!content_type) {
        USERVER_NAMESPACE::utils::LogErrorAndThrow("S3Api : Object head is missing `content-type` header");
    }

    auto req = api_methods::CopyObject(bucket_, key_from, bucket_to, key_to, *content_type);
    if (meta) {
        SaveMeta(req.headers, *meta);
    }
    return RequestApi(req, "copy_object");
}

std::string
ClientImpl::CopyObject(std::string_view key_from, std::string_view key_to, const std::optional<Meta>& meta) {
    return CopyObject(key_from, bucket_, key_to, meta);
}

ClientPtr GetS3Client(
    std::shared_ptr<S3Connection> s3conn,
    std::shared_ptr<authenticators::AccessKey> authenticator,
    std::string bucket
) {
    return GetS3Client(
        std::move(s3conn), std::static_pointer_cast<authenticators::Authenticator>(authenticator), std::move(bucket)
    );
}

ClientPtr GetS3Client(
    std::shared_ptr<S3Connection> s3conn,
    std::shared_ptr<authenticators::Authenticator> authenticator,
    std::string bucket
) {
    return std::static_pointer_cast<Client>(std::make_shared<ClientImpl>(s3conn, authenticator, bucket));
}

}  // namespace s3api

USERVER_NAMESPACE_END
