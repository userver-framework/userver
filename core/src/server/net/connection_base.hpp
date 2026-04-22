#pragma once

#include <memory>
#include <string>

#include <userver/engine/io/socket.hpp>
#include <userver/engine/task/task_with_result.hpp>

#include <server/http/http_request_parser.hpp>
#include <server/http/request_handler_base.hpp>
#include <server/net/connection_config.hpp>
#include <server/net/http_socket_reader.hpp>
#include <server/net/stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::net {

class ConnectionBase {
public:
    using HttpRequestPtr = std::shared_ptr<http::HttpRequest>;

    ConnectionBase(
        std::unique_ptr<engine::io::RwBase> peer_socket,
        std::string peer_name,
        const ConnectionConfig& config,
        const http::RequestHandlerBase& request_handler,
        Stats& stats
    );

    int GetFd() const;
    std::unique_ptr<engine::io::RwBase> ExtractSocket() noexcept;

protected:
    bool TryParseRequests(request::RequestParser& parser) noexcept;

    bool IsValid() const noexcept;
    engine::io::RwBase& GetSocket() noexcept;
    SocketBufferedReader& Reader() noexcept;
    void Shutdown() noexcept;

    void StopAcceptingRequests() noexcept;
    bool IsResponseChainValid() const noexcept;

    std::vector<HttpRequestPtr>& GetRequests() noexcept;
    std::string GetPeerName() const noexcept;
    bool ReadSome() noexcept;

    engine::TaskWithResult<void> HandleQueueItem(const std::shared_ptr<http::HttpRequest>& request) noexcept;

private:
    Stats& stats_;
    SocketBufferedReader reader_;
    std::unique_ptr<engine::io::RwBase> socket_;
    const ConnectionConfig& config_;
    const http::RequestHandlerBase& request_handler_;
    bool is_accepting_requests_{true};
    bool is_response_chain_valid_{true};
};

}  // namespace server::net

USERVER_NAMESPACE_END
