#pragma once

#include <exception>
#include <memory>
#include <string>
#include <vector>

#include <server/http/request_handler_base.hpp>
#include <server/net/connection_base.hpp>
#include <server/net/connection_config.hpp>
#include <server/net/stats.hpp>

#include <userver/engine/io/socket.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_config.hpp>
#include <userver/utils/slot_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {
class WaitAnyContext;
}

namespace server::http {
class Http2Session;
}

namespace server::net {

struct Http2SessionConfig;

class Http1IsNotSupported final : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class Http2Connection final : public ConnectionBase {
public:
    Http2Connection(
        const ConnectionConfig& config,
        const request::HttpRequestConfig& handler_defaults_config,
        std::unique_ptr<engine::io::RwBase> peer_socket,
        const engine::io::Sockaddr& remote_address,
        const http::RequestHandlerBase& request_handler,
        Stats& stats,
        request::ResponseDataAccounter& data_accounter
    );
    ~Http2Connection();

    void Process();

    std::shared_ptr<http::HttpRequest>& GetFirstHttp1Request();
    std::unique_ptr<engine::io::RwBase> ExtractSocket() noexcept;

private:
    using RequestTask = engine::TaskWithResult<void>;

    struct RequestTaskContext final {
        RequestTask task;
        HttpRequestPtr request;
        // Set for the task that runs a protocol tunnelled over an already answered
        // stream; such a task must not produce a response of its own.
        bool is_upgraded{false};
    };

    void ListenForRequests();
    RequestTaskContext StartRequestTask(std::shared_ptr<http::HttpRequest>&& request_ptr) noexcept;
    void StartAllRequestTasks(engine::WaitAnyContext& wait_any);
    void OnRequestTaskFinished(std::uint64_t event_id, engine::WaitAnyContext& wait_any) noexcept;
    void StartUpgradedTask(HttpRequestPtr&& request_ptr, engine::WaitAnyContext& wait_any) noexcept;
    void FinishUpgradedStream(const http::HttpRequest& request) noexcept;
    void SendResponse(http::HttpRequest& request) noexcept;

    std::unique_ptr<http::Http2Session> MakeParser();
    void EnsureHttp2();

    bool ShouldCloseConnection() const noexcept;
    bool IsIdle() const noexcept;

    const ConnectionConfig& config_;
    const request::HttpRequestConfig& handler_defaults_config_;
    const http::RequestHandlerBase& request_handler_;
    Stats& stats_;
    request::ResponseDataAccounter& data_accounter_;

    using HttpRequestPtr = std::shared_ptr<http::HttpRequest>;
    std::vector<HttpRequestPtr> pending_requests_;

    engine::io::Sockaddr remote_address_;
    std::unique_ptr<http::Http2Session> parser_;
    utils::SlotMap<RequestTaskContext, std::vector> handler_tasks_;
};

}  // namespace server::net

USERVER_NAMESPACE_END
