#pragma once

#include <memory>
#include <string>

#include <server/http/request_handler_base.hpp>
#include <server/net/connection_config.hpp>
#include <server/net/stats.hpp>

// TODO: use fwd
#include <server/http/http2_session.hpp>
#include <server/http/http_request_parser.hpp>
//
#include <userver/engine/io/socket.hpp>
#include <userver/server/request/request_base.hpp>
#include <userver/server/request/request_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::net {

struct Http2SessionConfig;

class Connection final {
public:
    enum class Type { kRequest, kMonitor };

    Connection(
        const ConnectionConfig& config,
        const request::HttpRequestConfig& handler_defaults_config,
        std::unique_ptr<engine::io::RwBase> peer_socket,
        const engine::io::Sockaddr& remote_address,
        const http::RequestHandlerBase& request_handler,
        std::shared_ptr<Stats> stats,
        request::ResponseDataAccounter& data_accounter
    );

    void Process();

    int Fd() const;

private:
    void Shutdown() noexcept;

    bool IsRequestTasksEmpty() const noexcept;

    void ListenForRequests() noexcept;
    void ProcessRequest(std::shared_ptr<request::RequestBase>&& request_ptr);
    bool WaitOnSocket(engine::Deadline deadline);

    engine::TaskWithResult<void> HandleQueueItem(const std::shared_ptr<request::RequestBase>& request) noexcept;
    void SendResponse(request::RequestBase& request);

    std::string Getpeername() const;

    bool ReadSome();
    std::unique_ptr<request::RequestParser> MakeParser(USERVER_NAMESPACE::http::HttpVersion ver);
    bool TryDetectHttpVersion(std::string& buffer, std::string_view req);

    const ConnectionConfig& config_;
    const request::HttpRequestConfig& handler_defaults_config_;
    std::unique_ptr<engine::io::RwBase> peer_socket_;
    const http::RequestHandlerBase& request_handler_;
    const std::shared_ptr<Stats> stats_;
    request::ResponseDataAccounter& data_accounter_;
    std::unique_ptr<request::RequestParser> parser_{nullptr};
    bool is_http2_parser_{false};

    using RequestBasePtr = std::shared_ptr<request::RequestBase>;
    std::vector<RequestBasePtr> pending_requests_;

    engine::io::Sockaddr remote_address_;
    std::string peer_name_;

    std::vector<char> pending_data_{};
    size_t pending_data_size_{0};

    bool is_accepting_requests_{true};
    bool is_response_chain_valid_{true};
};

}  // namespace server::net

USERVER_NAMESPACE_END
