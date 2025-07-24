#include <s3api/s3_connection.hpp>

#include <userver/clients/http/client.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/logging/log.hpp>
#include <userver/s3api/models/request.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api {

namespace {
clients::http::Request&
GetMethod(clients::http::Request& req, std::string_view url, std::string_view body, clients::http::HttpMethod method) {
    // TODO: Get rid of extra string_view->string conversion once
    // http::Request can work with string_view directly
    switch (method) {
        case clients::http::HttpMethod::kGet:
            return req.get(std::string{url});
        case clients::http::HttpMethod::kHead:
            return req.head(std::string{url});
        case clients::http::HttpMethod::kPost:
            return req.post(std::string{url}, std::string{body});
        case clients::http::HttpMethod::kPut:
            return req.put(std::string{url}, std::string{body});
        case clients::http::HttpMethod::kDelete:
            return req.delete_method(std::string{url});
        default:
            throw std::runtime_error("Unknown http method");
    }
}
}  // namespace

std::shared_ptr<clients::http::Response> S3Connection::RequestApi(Request& r, std::string_view method_name) {
    if (!r.bucket.empty()) {
        r.headers[USERVER_NAMESPACE::http::headers::kHost] = r.bucket + "." + api_url_;
    } else {
        r.headers[USERVER_NAMESPACE::http::headers::kHost] = api_url_;
    }
    LOG_DEBUG() << "S3 Host: " << r.headers[USERVER_NAMESPACE::http::headers::kHost];

    const std::string full_url = GetUrl(r, connection_type_);
    LOG_DEBUG() << "S3 file full_url: " << full_url;

    auto http_req =
        http_client_.CreateNotSignedRequest().timeout(config_.timeout).retry(config_.retries).headers(r.headers);

    if (config_.proxy.has_value()) {
        http_req.proxy(config_.proxy.value());
    }
    http_req.SetDestinationMetricName(
        fmt::format("{}/{}", r.headers[USERVER_NAMESPACE::http::headers::kHost], method_name)
    );
    std::shared_ptr<clients::http::Response> response;
    try {
        response = GetMethod(http_req, full_url, r.body, r.method).perform();
        response->raise_for_status();
    } catch (const clients::http::TimeoutException& e) {
        LOG_WARNING() << "S3Api : Http Request Timeout: " << full_url;
        throw;
    } catch (const clients::http::HttpException& exc) {
        LOG_INFO() << "S3Api : Http Request to mds failed " << response->body() << " : " << full_url;
        throw;
    }
    return response;
}

std::shared_ptr<clients::http::Response> S3Connection::DoStartApiRequest(const Request& r) const {
    auto headers = r.headers;
    if (!r.bucket.empty())
        headers[USERVER_NAMESPACE::http::headers::kHost] = r.bucket + "." + api_url_;
    else
        headers[USERVER_NAMESPACE::http::headers::kHost] = api_url_;

    const std::string full_url = GetUrl(r, connection_type_);

    auto http_req =
        http_client_.CreateNotSignedRequest().headers(headers).retry(config_.retries).timeout(config_.timeout);
    return GetMethod(http_req, full_url, r.body, r.method).perform();
}

std::shared_ptr<clients::http::Response> S3Connection::StartApiRequest(const Request& request) const {
    return DoStartApiRequest(request);
}

std::string S3Connection::GetUrl(const Request& r, S3ConnectionType connection_type) const {
    std::string full_url = api_url_;
    const bool is_localhost = api_url_.find("localhost:") != std::string::npos;
    const auto schema_pos = full_url.find("://");
    if (schema_pos == std::string::npos) {
        if (!is_localhost && !r.bucket.empty()) {
            full_url = fmt::format("{}.{}", r.bucket, api_url_);
        }
        if (connection_type == S3ConnectionType::kHttps) {
            full_url = "https://" + full_url;
        } else {
            full_url = "http://" + full_url;
        }
    } else {
        if (!is_localhost && !r.bucket.empty()) {
            const auto schema = full_url.substr(0, schema_pos);
            const auto schemaless_url = full_url.substr(schema_pos + 3);
            full_url = fmt::format("{}://{}.{}", schema, r.bucket, schemaless_url);
        }
    }
    if (!r.req.empty()) {
        full_url += '/';
        full_url += r.req;
    }
    return full_url;
}

std::shared_ptr<S3Connection> MakeS3Connection(
    clients::http::Client& http_client,
    S3ConnectionType connection_type,
    std::string server_url,
    const ConnectionCfg& params
) {
    return std::make_shared<S3Connection>(http_client, connection_type, std::move(server_url), params);
}

}  // namespace s3api

USERVER_NAMESPACE_END
