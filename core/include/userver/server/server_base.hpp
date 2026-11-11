#pragma once

/// @file userver/server/server_base.hpp
/// @brief Transport-independent HTTP server contract.

#include <string>
#include <string_view>
#include <vector>

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/server/congestion_control/limiter.hpp>
#include <userver/server/congestion_control/sensor.hpp>
#include <userver/server/http/http_status.hpp>

USERVER_NAMESPACE_BEGIN

namespace server {

struct ServerConfig;
namespace handlers {
class HttpHandlerBase;
}

/// @brief Transport-independent HTTP server capabilities used by handlers and congestion control.
class ServerBase : public congestion_control::Limitee, public congestion_control::RequestsSource {
public:
    virtual ~ServerBase() = default;

    /// @brief Return immutable defaults used when constructing handler configurations.
    virtual const server::ServerConfig& GetConfig() const = 0;

    /// @brief Return the common middleware pipeline for the selected endpoint.
    virtual std::vector<std::string> GetCommonMiddlewares(bool is_monitor) const = 0;

    /// @brief Return the endpoint default for the response hostname header.
    virtual bool GetResponseHostnameDefault(bool is_monitor) const = 0;

    /// @brief Register a fully constructed handler.
    virtual void AddHandler(const server::handlers::HttpHandlerBase&, engine::TaskProcessor&) = 0;

    /// @brief Configure the main endpoint's congestion-control rejection status.
    virtual void SetRpsRatelimitStatusCode(server::http::HttpStatus status) = 0;
};

}  // namespace server

USERVER_NAMESPACE_END
