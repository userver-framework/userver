#include "listener_impl.hpp"

#include <netinet/tcp.h>

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <system_error>

#include <server/net/create_socket.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/io/exception.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/io/tls_wrapper.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::net {

ListenerImpl::ListenerImpl(engine::TaskProcessor& task_processor,
                           std::shared_ptr<EndpointInfo> endpoint_info,
                           request::ResponseDataAccounter& data_accounter)
    : task_processor_(task_processor),
      endpoint_info_(std::move(endpoint_info)),
      stats_(std::make_shared<Stats>()),
      data_accounter_(data_accounter),
      socket_listener_task_(engine::CriticalAsyncNoSpan(
          task_processor_,
          [this](engine::io::Socket&& request_socket) {
            while (!engine::current_task::ShouldCancel()) {
              try {
                AcceptConnection(request_socket);
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
          CreateSocket(endpoint_info_->listener_config))) {}

ListenerImpl::~ListenerImpl() {
  LOG_TRACE() << "Stopping socket listener task";
  socket_listener_task_.SyncCancel();
  LOG_TRACE() << "Stopped socket listener task";

  connections_.CancelAndWait();
}

StatsAggregation ListenerImpl::GetStats() const {
  return StatsAggregation{*stats_};
}

void ListenerImpl::AcceptConnection(engine::io::Socket& request_socket) {
  auto peer_socket = request_socket.Accept({});

  const auto new_connection_count = ++endpoint_info_->connection_count;
  utils::FastScopeGuard guard{
      [this]() noexcept { --endpoint_info_->connection_count; }};

  if (new_connection_count > endpoint_info_->listener_config.max_connections) {
    LOG_LIMITED_WARNING() << endpoint_info_->GetDescription()
                          << " reached max_connections="
                          << endpoint_info_->listener_config.max_connections
                          << ", dropping connection #" << new_connection_count;
    return;
  }

  LOG_DEBUG() << "Accepted connection #" << new_connection_count << '/'
              << endpoint_info_->listener_config.max_connections;

  // In case of TaskProcessor overload we wish to keep the connection,
  // as reopening it is CPU consuming
  connections_.Detach(engine::CriticalAsyncNoSpan(
      task_processor_,
      [this](auto peer_socket, auto /*guard*/) {
        ProcessConnection(std::move(peer_socket));
      },
      std::move(peer_socket), std::move(guard)));
}

void ListenerImpl::ProcessConnection(engine::io::Socket peer_socket) {
  if (peer_socket.Getsockname().Domain() == engine::io::AddrDomain::kInet6 ||
      peer_socket.Getsockname().Domain() == engine::io::AddrDomain::kInet)
    peer_socket.SetOption(IPPROTO_TCP, TCP_NODELAY, 1);

  const auto fd = peer_socket.Fd();

  LOG_TRACE() << "Creating connection for fd " << fd;
  std::unique_ptr<engine::io::RwBase> socket;
  auto remote_address = peer_socket.Getpeername();
  if (endpoint_info_->listener_config.tls) {
    const auto& config = endpoint_info_->listener_config;
    socket = std::make_unique<engine::io::TlsWrapper>(
        engine::io::TlsWrapper::StartTlsServer(
            std::move(peer_socket), config.tls_cert, config.tls_private_key, {},
            config.tls_certificate_authorities));
  } else {
    socket = std::make_unique<engine::io::Socket>(std::move(peer_socket));
  }

  Connection connection_ptr(endpoint_info_->listener_config.connection_config,
                            endpoint_info_->listener_config.handler_defaults,
                            std::move(socket), std::move(remote_address),
                            endpoint_info_->request_handler, stats_,
                            data_accounter_);

  LOG_TRACE() << "Start connection processing for fd " << fd;
  connection_ptr.Process();
  LOG_TRACE() << "Finishing connection for fd " << fd;
}

}  // namespace server::net

USERVER_NAMESPACE_END
