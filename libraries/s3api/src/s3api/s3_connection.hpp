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

    S3Connection(const S3Connection& other)
        : std::enable_shared_from_this<S3Connection>(other),
          api_url_(other.api_url_),
          connection_type_(other.connection_type_),
          http_client_(other.http_client_),
          config_(other.config_) {}

    virtual ~S3Connection() = default;

    virtual std::shared_ptr<clients::http::Response> RequestApi(Request& r, std::string_view method_name);

    virtual std::shared_ptr<clients::http::Response> DoStartApiRequest(const Request& r) const;

    virtual std::shared_ptr<clients::http::Response> StartApiRequest(const Request& r) const;

    virtual std::string GetHost() const { return api_url_; }

    void UpdateConfig(ConnectionCfg&& config) { config_ = config; }

protected:
    std::string GetUrl(const Request& r, S3ConnectionType connection_type) const;

    ConnectionCfg GetConfig() const { return config_; }

    const std::string api_url_;
    S3ConnectionType connection_type_;
    clients::http::Client& http_client_;

private:
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
