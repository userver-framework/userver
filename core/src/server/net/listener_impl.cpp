#include <server/net/listener_impl.hpp>

#include <netinet/tcp.h>

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <system_error>

#include <server/net/create_socket.hpp>
#include <server/net/http2_connection.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/io/exception.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/io/tls_wrapper.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::net {

ListenerImpl::ListenerImpl(
    engine::TaskProcessor& task_processor,
    std::shared_ptr<EndpointInfo> endpoint_info,
    request::ResponseDataAccounter& data_accounter
)
    : task_processor_(task_processor),
      endpoint_info_(std::move(endpoint_info)),
      stats_(std::make_shared<Stats>()),
      data_accounter_(data_accounter)
{
    for (const auto& port : endpoint_info_->listener_config.ports) {
        socket_listener_tasks_.push_back(engine::CriticalAsyncNoTracing(
            task_processor_,
            [this](engine::io::Socket&& request_socket) {
                while (!engine::current_task::ShouldCancel()) {
                    try {
                        AcceptConnection(request_socket, endpoint_info_->listener_config.ports[0]);
                    } catch (const engine::io::IoCancelled&) {
                        break;
                    } catch (const std::exception& ex) {
                        LOG_ERROR() << "can't accept connection: " << ex;

                        // If we're out of files, allow other coroutines to close old
                        // connections
                        engine::Yield();
                    }
                }
            },
            CreateSocket(endpoint_info_->listener_config, port)
        ));
    }
}

ListenerImpl::~ListenerImpl() {
    StopListening();
    connections_.CancelAndWait();
}

StatsAggregation ListenerImpl::GetStats() const { return StatsAggregation{*stats_}; }

void ListenerImpl::StopListening() {
    if (socket_listener_tasks_.empty()) {
        return;
    }

    LOG_TRACE() << "Stopping socket listener task";
    for (auto& task : socket_listener_tasks_) {
        task.SyncCancel();
    }
    socket_listener_tasks_.clear();
    LOG_TRACE() << "Stopped socket listener task";
}

bool ListenerImpl::WaitForNoConnections(engine::Deadline deadline) const {
    return endpoint_info_->no_connections_event.WaitUntil(deadline, [this] {
        return endpoint_info_->connection_count == 0;
    });
}

void ListenerImpl::AcceptConnection(engine::io::Socket& request_socket, const PortConfig& port_config) {
    auto peer_socket = request_socket.Accept({});

    const auto new_connection_count = ++endpoint_info_->connection_count;
    utils::FastScopeGuard guard{[this]() noexcept {
        if (--endpoint_info_->connection_count == 0) {
            endpoint_info_->no_connections_event.Send();
        }
    }};

    if (new_connection_count > endpoint_info_->listener_config.max_connections) {
        LOG_LIMITED_WARNING()
            << " reached max_connections=" << endpoint_info_->listener_config.max_connections
            << ", dropping connection #" << new_connection_count;
        return;
    }

    LOG_DEBUG()
        << "Accepted connection #" << new_connection_count << '/' << endpoint_info_->listener_config.max_connections;

    // In case of TaskProcessor overload we wish to keep the connection,
    // as reopening it is CPU consuming
    connections_.Detach(engine::CriticalAsyncNoTracing(
        task_processor_,
        [this, &port_config](auto peer_socket, auto /*guard*/) {
            ProcessConnection(std::move(peer_socket), port_config);
        },
        std::move(peer_socket),
        std::move(guard)
    ));
}

void ListenerImpl::ProcessConnection(engine::io::Socket peer_socket, const PortConfig& port_config) {
    using USERVER_NAMESPACE::http::HttpVersion;

    if (peer_socket.Getsockname().Domain() == engine::io::AddrDomain::kInet6 ||
        peer_socket.Getsockname().Domain() == engine::io::AddrDomain::kInet)
    {
        peer_socket.SetOption(IPPROTO_TCP, TCP_NODELAY, 1);
    }

    const auto fd = peer_socket.Fd();

    LOG_TRACE() << "Creating connection for fd " << fd;
    std::unique_ptr<engine::io::RwBase> socket;
    auto remote_address = peer_socket.Getpeername();
    if (port_config.tls) {
        UASSERT(port_config.ssl_ctx.has_value());
        socket = std::make_unique<engine::io::TlsWrapper>(
            engine::io::TlsWrapper::StartTlsServer(std::move(peer_socket), port_config.ssl_ctx.value(), {})
        );
    } else {
        socket = std::make_unique<engine::io::Socket>(std::move(peer_socket));
    }

    LOG_TRACE() << "Start connection processing for fd " << fd;

    std::shared_ptr<http::HttpRequest> first_request{};
    std::unique_ptr<engine::io::RwBase> extracted_socket{};
    UASSERT(stats_);
    if (endpoint_info_->listener_config.connection_config.http_version == USERVER_NAMESPACE::http::HttpVersion::k2) {
        Http2Connection connection(
            endpoint_info_->listener_config.connection_config,
            endpoint_info_->listener_config.handler_defaults,
            std::move(socket),
            remote_address,
            endpoint_info_->request_handler,
            *stats_,
            data_accounter_
        );
        try {
            connection.Process();
            LOG_TRACE() << "Finishing connection for fd " << fd;
            return;
        } catch (const Http1IsNotSupported& e) {
            first_request = std::move(connection.GetFirstHttp1Request());
            extracted_socket = connection.ExtractSocket();
        } catch (const std::exception& e) {
            LOG_INFO() << "Closing idle connection: " << e;
            return;
        }
    }
    if (!extracted_socket) {
        extracted_socket = std::move(socket);
    }
    Http1Connection connection(
        endpoint_info_->listener_config.connection_config,
        endpoint_info_->listener_config.handler_defaults,
        std::move(extracted_socket),
        std::move(remote_address),
        endpoint_info_->request_handler,
        *stats_,
        data_accounter_
    );
    if (first_request) {
        // If HTTP/1.1 request was read on HTTP/2.0 connection, we prefer to switch to a pure HTTP/1.1 connection
        // and handle it here.
        connection.ProcessRequest(std::move(first_request));
    }
    connection.Process();
    LOG_TRACE() << "Finishing connection for fd " << fd;
}

}  // namespace server::net

USERVER_NAMESPACE_END
