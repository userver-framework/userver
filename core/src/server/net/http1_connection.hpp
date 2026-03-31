#pragma once

#include <memory>
#include <string>

#include <server/http/http_request_parser.hpp>
#include <server/http/request_handler_base.hpp>
#include <server/net/connection_base.hpp>
#include <server/net/connection_config.hpp>
#include <server/net/stats.hpp>
#include <server/request/request_parser.hpp>
#include <userver/server/http/http_request.hpp>

#include <userver/engine/io/socket.hpp>
#include <userver/server/request/request_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::net {

class Http1Connection final : public ConnectionBase {
public:
    Http1Connection(
        const ConnectionConfig& config,
        const request::HttpRequestConfig& handler_defaults_config,
        std::unique_ptr<engine::io::RwBase> peer_socket,
        const engine::io::Sockaddr& remote_address,
        const http::RequestHandlerBase& request_handler,
        Stats& stats,
        request::ResponseDataAccounter& data_accounter
    );

    void Process();

    void ProcessRequest(std::shared_ptr<http::HttpRequest>&& request_ptr);

private:
    bool IsRequestTasksEmpty() const noexcept;

    void ListenForRequests() noexcept;

    void SendResponse(http::HttpRequest& request);

    const ConnectionConfig& config_;
    const request::HttpRequestConfig& handler_defaults_config_;
    const http::RequestHandlerBase& request_handler_;
    Stats& stats_;
    request::ResponseDataAccounter& data_accounter_;
    engine::io::Sockaddr remote_address_;
};

}  // namespace server::net

USERVER_NAMESPACE_END
