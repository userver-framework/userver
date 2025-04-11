#pragma once

#include <optional>
#include <string>

#include <userver/clients/http/request.hpp>
#include <userver/clients/http/response.hpp>
#include <userver/utils/str_icase.hpp>

#include <userver/s3api/clients/s3api.hpp>
#include <userver/s3api/models/fwd.hpp>
#include <userver/s3api/models/s3api_connection_type.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {
class Client;
}

namespace s3api {

// Represents a connection to s3 api
// includes an url to the api and a http interface
class S3Connection : public std::enable_shared_from_this<S3Connection> {
public:
    S3Connection(
        clients::http::Client& http_client,
        S3ConnectionType connection_type,
        std::string server_url,
        const ConnectionCfg& params
    )
        : api_url_(std::move(server_url)),
          connection_type_(connection_type),
          http_client_(http_client),
          config_(params) {}

    S3Connection(const S3Connection& other) = default;

    ~S3Connection() = default;

    std::shared_ptr<clients::http::Response> RequestApi(Request& r, std::string_view method_name);

    std::shared_ptr<clients::http::Response> DoStartApiRequest(const Request& r) const;

    std::shared_ptr<clients::http::Response> StartApiRequest(const Request& r) const;

    std::string GetHost() const { return api_url_; }

    void UpdateConfig(ConnectionCfg&& config) { config_ = config; }

private:
    std::string GetUrl(const Request& r, S3ConnectionType connection_type) const;

    ConnectionCfg GetConfig() const { return config_; }

    const std::string api_url_;
    S3ConnectionType connection_type_;
    clients::http::Client& http_client_;
    ConnectionCfg config_;
};

std::shared_ptr<S3Connection> MakeS3Connection(
    clients::http::Client& http_client,
    S3ConnectionType connection_type,
    std::string server_url,
    const ConnectionCfg& params
);

}  // namespace s3api

USERVER_NAMESPACE_END
