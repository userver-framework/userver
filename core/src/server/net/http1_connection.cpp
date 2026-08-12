#include <server/net/http1_connection.hpp>

#include <array>
#include <system_error>
#include <vector>

#include <server/http/request_handler_base.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/exception.hpp>
#include <userver/engine/io/exception.hpp>
#include <userver/engine/io/tls_wrapper.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/wait_any.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/request/request_config.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::net {

Http1Connection::Http1Connection(
    const ConnectionConfig& config,
    const request::HttpRequestConfig& handler_defaults_config,
    std::unique_ptr<engine::io::RwBase> peer_socket,
    const engine::io::Sockaddr& remote_address,
    const http::RequestHandlerBase& request_handler,
    Stats& stats,
    request::ResponseDataAccounter& data_accounter
)
    : ConnectionBase(std::move(peer_socket), remote_address.PrimaryAddressString(), config, request_handler, stats),
      config_(config),
      handler_defaults_config_(handler_defaults_config),
      request_handler_(request_handler),
      stats_(stats),
      data_accounter_(data_accounter),
      remote_address_(remote_address)
{
    LOG_DEBUG() << "Incoming connection from " << GetPeerName() << ", fd " << GetFd();

    ++stats_.active_connections;
    ++stats_.connections_created;
}

void Http1Connection::Process() {
    LOG_TRACE() << "Starting socket listener for fd " << GetFd();

    ListenForRequests();

    Shutdown();
}

void Http1Connection::ListenForRequests() noexcept {
    using RequestBasePtr = std::shared_ptr<http::HttpRequest>;

    try {
        std::vector<RequestBasePtr> pending_requests;

        http::HttpRequestParser request_parser(
            request_handler_.GetHandlerInfoIndex(),
            handler_defaults_config_,
            [&pending_requests](RequestBasePtr&& request_ptr) { pending_requests.push_back(std::move(request_ptr)); },
            stats_.parser_stats,
            data_accounter_,
            remote_address_
        );

        while (TryParseRequests(request_parser)) {
            for (auto&& request : pending_requests) {
                ProcessRequest(std::move(request));
            }
            pending_requests.clear();
        }
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error while receiving from peer " << GetPeerName() << " on fd " << GetFd() << ": " << ex;
    }
}

void Http1Connection::ProcessRequest(std::shared_ptr<http::HttpRequest>&& request_ptr) {
    if (request_ptr->IsFinal()) {
        StopAcceptingRequests();
    }

    stats_.active_request_count.Add(1);

    auto task = HandleQueueItem(request_ptr);
    SendResponse(*request_ptr);

    if (request_ptr->IsUpgradeWebsocket()) {
        request_ptr->DoUpgrade(ExtractSocket(), std::move(remote_address_));
        StopAcceptingRequests();
    }
}

void Http1Connection::SendResponse(http::HttpRequest& request) {
    auto& response = request.GetHttpResponse();
    UASSERT(!response.IsSent());
    request.SetStartSendResponseTime();
    if (IsResponseChainValid() && IsValid()) {
        try {
            // Might be a stream reading or a fully constructed response
            response.SendResponse(GetSocket());
        } catch (const engine::io::IoSystemError& ex) {
            auto log_level = ex.Code().value() == EPIPE ? logging::Level::kWarning : logging::Level::kError;
            LOG(log_level) << "I/O error while sending data: " << ex;
            response.SetSendFailed(std::chrono::steady_clock::now());
        } catch (const std::exception& ex) {
            LOG_ERROR() << "Error while sending data: " << ex;
            response.SetSendFailed(std::chrono::steady_clock::now());
        }
    } else {
        response.SetSendFailed(std::chrono::steady_clock::now());
    }
    request.SetFinishSendResponseTime();
    stats_.active_request_count.Subtract(1);
    ++stats_.requests_processed_count;

    request.WriteAccessLogs(request_handler_.LoggerAccess(), request_handler_.LoggerAccessTskv(), GetPeerName());
}

}  // namespace server::net

USERVER_NAMESPACE_END
